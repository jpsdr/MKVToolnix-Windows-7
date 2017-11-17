/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Matroska reader

   Written by Moritz Bunkus <moritz@bunkus.org>.
   Modified by Steve Lhomme <steve.lhomme@free.fr>.
*/

#include "common/common_pch.h"

#include <cmath>

#include <ebml/EbmlContexts.h>
#include <ebml/EbmlHead.h>
#include <ebml/EbmlStream.h>
#include <ebml/EbmlSubHead.h>
#include <ebml/EbmlVoid.h>
#include <matroska/FileKax.h>

#include <matroska/KaxAttached.h>
#include <matroska/KaxAttachments.h>
#include <matroska/KaxBlock.h>
#include <matroska/KaxBlockData.h>
#include <matroska/KaxChapters.h>
#include <matroska/KaxCluster.h>
#include <matroska/KaxClusterData.h>
#include <matroska/KaxContexts.h>
#include <matroska/KaxInfo.h>
#include <matroska/KaxInfoData.h>
#include <matroska/KaxSeekHead.h>
#include <matroska/KaxSegment.h>
#include <matroska/KaxTag.h>
#include <matroska/KaxTags.h>
#include <matroska/KaxTracks.h>
#include <matroska/KaxTrackAudio.h>
#include <matroska/KaxTrackVideo.h>

#include "avilib.h"
#include "common/alac.h"
#include "common/at_scope_exit.h"
#include "common/chapters/chapters.h"
#include "common/codec.h"
#include "common/date_time.h"
#include "common/debugging.h"
#include "common/ebml.h"
#include "common/endian.h"
#include "common/hacks.h"
#include "common/iso639.h"
#include "common/ivf.h"
#include "common/kax_analyzer.h"
#include "common/mm_io.h"
#include "common/strings/formatting.h"
#include "common/strings/parsing.h"
#include "common/strings/utf8.h"
#include "common/tags/tags.h"
#include "common/id_info.h"
#include "common/vobsub.h"
#include "input/r_matroska.h"
#include "merge/file_status.h"
#include "merge/input_x.h"
#include "merge/output_control.h"
#include "output/p_aac.h"
#include "output/p_ac3.h"
#include "output/p_alac.h"
#include "output/p_avc.h"
#include "output/p_avc_es.h"
#include "output/p_dirac.h"
#include "output/p_dts.h"
#include "output/p_dvbsub.h"
#if defined(HAVE_FLAC_FORMAT_H)
# include "output/p_flac.h"
#endif
#include "output/p_hdmv_pgs.h"
#include "output/p_hdmv_textst.h"
#include "output/p_hevc.h"
#include "output/p_hevc_es.h"
#include "output/p_kate.h"
#include "output/p_mp3.h"
#include "output/p_mpeg1_2.h"
#include "output/p_mpeg4_p2.h"
#include "output/p_opus.h"
#include "output/p_passthrough.h"
#include "output/p_pcm.h"
#include "output/p_textsubs.h"
#include "output/p_theora.h"
#include "output/p_truehd.h"
#include "output/p_tta.h"
#include "output/p_video_for_windows.h"
#include "output/p_vobbtn.h"
#include "output/p_vobsub.h"
#include "output/p_vorbis.h"
#include "output/p_vc1.h"
#include "output/p_vpx.h"
#include "output/p_wavpack.h"
#include "output/p_webvtt.h"

using namespace libmatroska;

#define MAP_TRACK_TYPE(c) (  (c) == 'a' ? track_audio   \
                           : (c) == 'b' ? track_buttons \
                           : (c) == 'v' ? track_video   \
                           :              track_subtitle)
#define MAP_TRACK_TYPE_STRING(c) (  (c) == '?' ? Y("unknown")   \
                                  : (c) == 'a' ? Y("audio")     \
                                  : (c) == 'b' ? Y("buttons")   \
                                  : (c) == 'v' ? Y("video")     \
                                  :              Y("subtitle"))

#define in_parent(p) \
  (!p->IsFiniteSize() || (m_in->getFilePointer() < (p->GetElementPosition() + p->HeadSize() + p->GetSize())))

#define is_ebmlvoid(e) (Is<EbmlVoid>(e))

#define MAGIC_MKV 0x1a45dfa3

void
kax_track_t::handle_packetizer_display_dimensions() {
  // If user hasn't set an aspect ratio via the command line and the
  // source file provides display width/height paramaters then use
  // these and signal the packetizer not to extract the dimensions
  // from the bitstream.
  if ((0 != v_dwidth) && (0 != v_dheight))
    ptzr_ptr->set_video_display_dimensions(v_dwidth, v_dheight, OPTION_SOURCE_CONTAINER);
}

void
kax_track_t::handle_packetizer_pixel_cropping() {
  if ((0 < v_pcleft) || (0 < v_pctop) || (0 < v_pcright) || (0 < v_pcbottom))
    ptzr_ptr->set_video_pixel_cropping(v_pcleft, v_pctop, v_pcright, v_pcbottom, OPTION_SOURCE_CONTAINER);
}

void
kax_track_t::handle_packetizer_colour() {
  if (v_colour_matrix != -1)
    ptzr_ptr->set_video_colour_matrix(v_colour_matrix, OPTION_SOURCE_CONTAINER);
  if (v_bits_per_channel != -1)
    ptzr_ptr->set_video_bits_per_channel(v_bits_per_channel, OPTION_SOURCE_CONTAINER);
  if (v_chroma_subsample.hori != -1 || v_chroma_subsample.vert != -1)
    ptzr_ptr->set_video_chroma_subsample(v_chroma_subsample, OPTION_SOURCE_CONTAINER);
  if (v_cb_subsample.hori != -1 || v_cb_subsample.vert != -1)
    ptzr_ptr->set_video_cb_subsample(v_cb_subsample, OPTION_SOURCE_CONTAINER);
  if (v_chroma_siting.hori != -1 || v_chroma_siting.vert != -1)
    ptzr_ptr->set_video_chroma_siting(v_chroma_siting, OPTION_SOURCE_CONTAINER);
  if (v_colour_range != -1)
    ptzr_ptr->set_video_colour_range(v_colour_range, OPTION_SOURCE_CONTAINER);
  if (v_transfer_character != -1)
    ptzr_ptr->set_video_colour_transfer_character(v_transfer_character, OPTION_SOURCE_CONTAINER);
  if (v_colour_primaries != -1)
    ptzr_ptr->set_video_colour_primaries(v_colour_primaries, OPTION_SOURCE_CONTAINER);
  if (v_max_cll != -1)
    ptzr_ptr->set_video_max_cll(v_max_cll, OPTION_SOURCE_CONTAINER);
  if (v_max_fall != -1)
    ptzr_ptr->set_video_max_fall(v_max_fall, OPTION_SOURCE_CONTAINER);
  if (   (v_chroma_coordinates.red_x   != -1) || (v_chroma_coordinates.red_y   != -1)
      || (v_chroma_coordinates.green_x != -1) || (v_chroma_coordinates.green_y != -1)
      || (v_chroma_coordinates.blue_x  != -1) || (v_chroma_coordinates.blue_y  != -1)) {
    ptzr_ptr->set_video_chroma_coordinates(v_chroma_coordinates, OPTION_SOURCE_CONTAINER);
  }
  if (v_white_colour_coordinates.x != -1 || v_white_colour_coordinates.y != -1)
    ptzr_ptr->set_video_white_colour_coordinates(v_white_colour_coordinates, OPTION_SOURCE_CONTAINER);
  if (v_max_luminance != -1)
    ptzr_ptr->set_video_max_luminance(v_max_luminance, OPTION_SOURCE_CONTAINER);
  if (v_min_luminance != -1)
    ptzr_ptr->set_video_min_luminance(v_min_luminance, OPTION_SOURCE_CONTAINER);

  if (v_projection_type)
    ptzr_ptr->set_video_projection_type(*v_projection_type, OPTION_SOURCE_CONTAINER);
  if (v_projection_private)
    ptzr_ptr->set_video_projection_private(v_projection_private, OPTION_SOURCE_CONTAINER);
  if (v_projection_pose_yaw)
    ptzr_ptr->set_video_projection_pose_yaw(*v_projection_pose_yaw, OPTION_SOURCE_CONTAINER);
  if (v_projection_pose_pitch)
    ptzr_ptr->set_video_projection_pose_pitch(*v_projection_pose_pitch, OPTION_SOURCE_CONTAINER);
  if (v_projection_pose_roll)
    ptzr_ptr->set_video_projection_pose_roll(*v_projection_pose_roll, OPTION_SOURCE_CONTAINER);
}

void
kax_track_t::handle_packetizer_field_order() {
  if (-1 != v_field_order)
    ptzr_ptr->set_video_field_order(v_field_order, OPTION_SOURCE_CONTAINER);
}

void
kax_track_t::handle_packetizer_stereo_mode() {
  if (stereo_mode_c::unspecified != v_stereo_mode)
    ptzr_ptr->set_video_stereo_mode(v_stereo_mode, OPTION_SOURCE_CONTAINER);
}

void
kax_track_t::handle_packetizer_pixel_dimensions() {
  if ((0 == v_width) || (0 == v_height))
    return;

  ptzr_ptr->set_video_pixel_width(v_width);
  ptzr_ptr->set_video_pixel_height(v_height);
}

void
kax_track_t::handle_packetizer_default_duration() {
  if (0 != default_duration)
    ptzr_ptr->set_track_default_duration(default_duration);
}

void
kax_track_t::handle_packetizer_output_sampling_freq() {
  if (0 != a_osfreq)
    ptzr_ptr->set_audio_output_sampling_freq(a_osfreq);
}

void
kax_track_t::handle_packetizer_codec_delay() {
  if (codec_delay.valid() && (codec_delay.to_ns() > 0))
    ptzr_ptr->set_codec_delay(codec_delay);
}

/* Fix display dimension parameters

   Certain Matroska muxers abuse the DisplayWidth/DisplayHeight
   parameters for only storing an aspect ratio. These values are
   usually very small, e.g. 16/9. Fix them so that the quotient is
   kept but the values are in the range of the PixelWidth/PixelHeight
   elements.
 */
void
kax_track_t::fix_display_dimension_parameters() {
  if ((0 != v_display_unit) || ((8 * v_dwidth) > v_width) || ((8 * v_dheight) > v_height))
    return;

  if (boost::math::gcd(v_dwidth, v_dheight) != 1)
    return;

  // max shrinking was applied, ie x264 style
  if (v_dwidth > v_dheight) {
    if (((v_height * v_dwidth) % v_dheight) == 0) { // only if we get get an exact count of pixels
      v_dwidth  = v_height * v_dwidth / v_dheight;
      v_dheight = v_height;
    }
  } else if (((v_width * v_dheight) % v_dwidth) == 0) {
    v_dwidth  = v_width;
    v_dheight = v_width * v_dheight / v_dwidth;
  }
}

void
kax_track_t::add_track_tags_to_identification(mtx::id::info_c &info) {
  if (!tags)
    return;

  for (auto const &tag_elt : *tags) {
    auto tag = dynamic_cast<KaxTag *>(tag_elt);
    if (!tag)
      continue;

    for (auto const &simple_tag_elt : *tag) {
      auto simple_tag = dynamic_cast<KaxTagSimple *>(simple_tag_elt);
      if (!simple_tag)
        continue;

      auto name  = mtx::tags::get_simple_name(*simple_tag);
      auto value = mtx::tags::get_simple_value(*simple_tag);

      if (!name.empty())
        info.add((boost::format("tag_%1%") % balg::to_lower_copy(name)).str(), value);
}
  }
}

void
kax_track_t::discard_track_statistics_tags() {
  if (!tags)
    return;

  mtx::tags::remove_track_statistics(tags, track_uid);

  if (!tags->ListSize()) {
    delete tags;
    tags = nullptr;
  }
}

/*
   Probes a file by simply comparing the first four bytes to the EBML
   head signature.
*/
int
kax_reader_c::probe_file(mm_io_c *in,
                         uint64_t size) {
  unsigned char data[4];

  if (4 > size)
    return 0;
  try {
    in->setFilePointer(0, seek_beginning);
    if (in->read(data, 4) != 4)
      return 0;
    in->setFilePointer(0, seek_beginning);
  } catch (...) {
    return 0;
  }
  if (get_uint32_be(data) != MAGIC_MKV)
    return 0;
  return 1;
}

kax_reader_c::kax_reader_c(const track_info_c &ti,
                           const mm_io_cptr &in)
  : generic_reader_c(ti, in)
  , m_segment_duration(0)
  , m_last_timestamp(0)
  , m_first_timestamp(-1)
  , m_global_timestamp_offset{}
  , m_writing_app_ver(-1)
  , m_attachment_id(0)
  , m_file_status(FILE_STATUS_MOREDATA)
  , m_opus_experimental_warning_shown{}
  , m_regenerate_chapter_uids{}
{
  init_l1_position_storage(m_deferred_l1_positions);
  init_l1_position_storage(m_handled_l1_positions);
}

kax_reader_c::~kax_reader_c() {
}

void
kax_reader_c::init_l1_position_storage(deferred_positions_t &storage) {
  storage[dl1t_attachments] = std::vector<int64_t>();
  storage[dl1t_chapters]    = std::vector<int64_t>();
  storage[dl1t_tags]        = std::vector<int64_t>();
  storage[dl1t_tracks]      = std::vector<int64_t>();
  storage[dl1t_seek_head]   = std::vector<int64_t>();
}

bool
kax_reader_c::packets_available() {
  for (auto &track : m_tracks)
    if ((-1 != track->ptzr) && !PTZR(track->ptzr)->packet_available())
      return false;

  return !m_tracks.empty();
}

kax_track_t *
kax_reader_c::find_track_by_num(uint64_t n,
                                kax_track_t *c) {
  auto itr = brng::find_if(m_tracks, [n, c](auto &track) { return (track->track_number == n) && (track.get() != c); });
  return itr == m_tracks.end() ? nullptr : itr->get();
}

kax_track_t *
kax_reader_c::find_track_by_uid(uint64_t uid,
                                kax_track_t *c) {
  auto itr = brng::find_if(m_tracks, [uid, c](auto &track) { return (track->track_uid == uid) && (track.get() != c); });
  return itr == m_tracks.end() ? nullptr : itr->get();
}

bool
kax_reader_c::unlace_vorbis_private_data(kax_track_t *t,
                                         unsigned char *buffer,
                                         int size) {
  try {
    memory_cptr temp(new memory_c(buffer, size, false));
    std::vector<memory_cptr> blocks = unlace_memory_xiph(temp);
    if (blocks.size() != 3)
      return false;

    for (int i = 0; 3 > i; ++i) {
      t->headers[i]      = blocks[i]->get_buffer();
      t->header_sizes[i] = blocks[i]->get_size();
    }

  } catch (...) {
    return false;
  }

  return true;
}

bool
kax_reader_c::verify_acm_audio_track(kax_track_t *t) {
  if (!t->private_data || (sizeof(alWAVEFORMATEX) > t->private_data->get_size())) {
    if (verbose)
      mxwarn(boost::format(Y("matroska_reader: The CodecID for track %1% is '%2%', but there was no WAVEFORMATEX struct present. "
                             "Therefore we don't have a format ID to identify the audio codec used.\n")) % t->tnum % MKV_A_ACM);
    return false;

  }

  auto wfe       = reinterpret_cast<alWAVEFORMATEX *>(t->private_data->get_buffer());
  t->a_formattag = get_uint16_le(&wfe->w_format_tag);
  t->codec       = codec_c::look_up_audio_format(t->a_formattag);

  if (t->codec.is(codec_c::type_e::A_VORBIS) && (!unlace_vorbis_private_data(t, t->private_data->get_buffer() + sizeof(alWAVEFORMATEX), t->private_data->get_size() - sizeof(alWAVEFORMATEX)))) {
    // Force the passthrough packetizer to be used if the data behind
    // the WAVEFORMATEX does not contain valid laced Vorbis headers.
    t->codec = codec_c{};
    return true;
  }

  t->ms_compat = 1;
  uint32_t u   = get_uint32_le(&wfe->n_samples_per_sec);

  if (static_cast<uint32_t>(t->a_sfreq) != u) {
    if (verbose)
      mxwarn(boost::format(Y("matroska_reader: (MS compatibility mode for track %1%) Matroska says that there are %2% samples per second, "
                             "but WAVEFORMATEX says that there are %3%.\n")) % t->tnum % static_cast<int>(t->a_sfreq) % u);
    if (0.0 == t->a_sfreq)
      t->a_sfreq = static_cast<double>(u);
  }

  u = get_uint16_le(&wfe->n_channels);
  if (t->a_channels != u) {
    if (verbose)
      mxwarn(boost::format(Y("matroska_reader: (MS compatibility mode for track %1%) Matroska says that there are %2% channels, "
                             "but the WAVEFORMATEX says that there are %3%.\n")) % t->tnum % t->a_channels % u);
    if (0 == t->a_channels)
      t->a_channels = u;
  }

  u = get_uint16_le(&wfe->w_bits_per_sample);
  if (t->a_bps != u) {
    if (verbose && t->codec.is(codec_c::type_e::A_PCM))
      mxwarn(boost::format(Y("matroska_reader: (MS compatibility mode for track %1%) Matroska says that there are %2% bits per sample, "
                             "but the WAVEFORMATEX says that there are %3%.\n")) % t->tnum % t->a_bps % u);
    if (0 == t->a_bps)
      t->a_bps = u;
  }

  return true;
}

bool
kax_reader_c::verify_alac_audio_track(kax_track_t *t) {
  if (t->private_data && (sizeof(mtx::alac::codec_config_t) <= t->private_data->get_size()))
    return true;

  if (verbose)
    mxwarn(boost::format(Y("matroska_reader: The CodecID for track %1% is '%2%', but the private codec data does not contain valid headers.\n")) % t->tnum % MKV_A_VORBIS);

  return false;
}

bool
kax_reader_c::verify_dts_audio_track(kax_track_t *t) {
  try {
    read_first_frames(t, 5);

    mtx::bytes::buffer_c buffer;
    for (auto &frame : t->first_frames_data)
      buffer.add(*frame);

    if (-1 == mtx::dts::find_header(buffer.get_buffer(), buffer.get_size(), t->dts_header))
      return false;

    t->codec.set_specialization(t->dts_header.get_codec_specialization());

  } catch (...) {
    return false;
  }

  return true;
}

#if defined(HAVE_FLAC_FORMAT_H)
bool
kax_reader_c::verify_flac_audio_track(kax_track_t *) {
  return true;
}

#else

bool
kax_reader_c::verify_flac_audio_track(kax_track_t *t) {
  mxwarn(boost::format(Y("matroska_reader: mkvmerge was not compiled with FLAC support. Ignoring track %1%.\n")) % t->tnum);
  return false;
}
#endif

bool
kax_reader_c::verify_vorbis_audio_track(kax_track_t *t) {
  if (!t->private_data || !unlace_vorbis_private_data(t, t->private_data->get_buffer(), t->private_data->get_size())) {
    if (verbose)
      mxwarn(boost::format(Y("matroska_reader: The CodecID for track %1% is '%2%', but the private codec data does not contain valid headers.\n")) % t->tnum % MKV_A_VORBIS);
    return false;
  }

  // mkvmerge around 0.6.x had a bug writing a default duration
  // for Vorbis m_tracks but not the correct durations for the
  // individual packets. This comes back to haunt us because
  // when regenerating the timestamps from lacing those durations
  // are used and add up to too large a value. The result is the
  // usual "timestamp < m_last_timestamp" message.
  // Workaround: do not use durations for such m_tracks.
  if ((m_writing_app == "mkvmerge") && (-1 != m_writing_app_ver) && (0x07000000 > m_writing_app_ver))
    t->ignore_duration_hack = true;

  return true;
}

bool
kax_reader_c::verify_opus_audio_track(kax_track_t *t) {
  if (!t->private_data || !t->private_data->get_size()) {
    if (verbose)
      mxwarn(boost::format(Y("matroska_reader: The CodecID for track %1% is '%2%', but the private codec data does not contain valid headers.\n")) % t->tnum % MKV_A_OPUS);
    return false;
  }

  return true;
}

bool
kax_reader_c::verify_truehd_audio_track(kax_track_t *t) {
  try {
    read_first_frames(t, 5);

    truehd_parser_c parser;
    for (auto &frame : t->first_frames_data)
      parser.add_data(frame->get_buffer(), frame->get_size());

    while (parser.frame_available()) {
      auto frame = parser.get_next_frame();

      if (frame->is_ac3() || !frame->is_sync())
        continue;

      t->codec = frame->codec();

      return true;
    }

  } catch (...) {
  }

  return false;
}

bool
kax_reader_c::verify_ac3_audio_track(kax_track_t *t) {
  try {
    read_first_frames(t, 5);

    mtx::ac3::parser_c parser;
    for (auto &frame : t->first_frames_data)
      parser.add_bytes(frame->get_buffer(), frame->get_size());

    if (parser.frame_available()) {
      t->codec = parser.get_frame().get_codec();
      return true;
    }

  } catch (...) {
  }

  return false;
}

void
kax_reader_c::verify_audio_track(kax_track_t *t) {
  if (t->codec_id.empty())
    return;

  bool is_ok = true;
  if (t->codec_id == MKV_A_ACM)
    is_ok = verify_acm_audio_track(t);

  else {
    t->codec = codec_c::look_up(t->codec_id);

    if (t->codec.is(codec_c::type_e::A_AC3))
      is_ok = verify_ac3_audio_track(t);
    else if (t->codec.is(codec_c::type_e::A_ALAC))
      is_ok = verify_alac_audio_track(t);
    else if (t->codec.is(codec_c::type_e::A_VORBIS))
      is_ok = verify_vorbis_audio_track(t);
    else if (t->codec.is(codec_c::type_e::A_FLAC))
      is_ok = verify_flac_audio_track(t);
    else if (t->codec.is(codec_c::type_e::A_OPUS))
      is_ok = verify_opus_audio_track(t);
    else if (t->codec.is(codec_c::type_e::A_DTS))
      is_ok = verify_dts_audio_track(t);
    else if (t->codec.is(codec_c::type_e::A_TRUEHD))
      is_ok = verify_truehd_audio_track(t);
  }

  if (!is_ok)
    return;

  if (0.0 == t->a_sfreq)
    t->a_sfreq = 8000.0;

  if (0 == t->a_channels)
    t->a_channels = 1;

  // This track seems to be ok.
  t->ok = 1;
}

bool
kax_reader_c::verify_mscomp_video_track(kax_track_t *t) {
  if (!t->private_data || (sizeof(alBITMAPINFOHEADER) > t->private_data->get_size())) {
    if (verbose)
      mxwarn(boost::format(Y("matroska_reader: The CodecID for track %1% is '%2%', but there was no BITMAPINFOHEADER struct present. "
                             "Therefore we don't have a FourCC to identify the video codec used.\n"))
             % t->tnum % MKV_V_MSCOMP);
    return false;
  }

  t->ms_compat = 1;
  auto bih     = reinterpret_cast<alBITMAPINFOHEADER *>(t->private_data->get_buffer());
  auto u       = get_uint32_le(&bih->bi_width);

  if (t->v_width != u) {
    if (verbose)
      mxwarn(boost::format(Y("matroska_reader: (MS compatibility mode, track %1%) Matroska says video width is %2%, but the BITMAPINFOHEADER says %3%.\n"))
             % t->tnum % t->v_width % u);
    if (0 == t->v_width)
      t->v_width = u;
  }

  u = get_uint32_le(&bih->bi_height);
  if (t->v_height != u) {
    if (verbose)
      mxwarn(boost::format(Y("matroska_reader: (MS compatibility mode, track %1%) Matroska says video height is %2%, but the BITMAPINFOHEADER says %3%.\n"))
             % t->tnum % t->v_height % u);
    if (0 == t->v_height)
      t->v_height = u;
  }

  memcpy(t->v_fourcc, &bih->bi_compression, 4);
  t->v_fourcc[4] = 0;
  t->codec       = codec_c::look_up(t->v_fourcc);

  return true;
}

bool
kax_reader_c::verify_theora_video_track(kax_track_t *t) {
  if (t->private_data)
    return true;

  if (verbose)
    mxwarn(boost::format(Y("matroska_reader: The CodecID for track %1% is '%2%', but there was no codec private headers.\n")) % t->tnum % MKV_V_THEORA);

  return false;
}

void
kax_reader_c::verify_video_track(kax_track_t *t) {
  if (t->codec_id.empty())
    return;

  bool is_ok = true;
  if (t->codec_id == MKV_V_MSCOMP)
    is_ok = verify_mscomp_video_track(t);

  else {
    t->codec = codec_c::look_up(t->codec_id);

    if (t->codec.is(codec_c::type_e::V_THEORA))
      is_ok = verify_theora_video_track(t);
  }

  if (!is_ok)
    return;

  if (0 == t->v_width) {
    if (verbose)
      mxwarn(boost::format(Y("matroska_reader: The width for track %1% was not set.\n")) % t->tnum);
    return;
  }

  if (0 == t->v_height) {
    if (verbose)
      mxwarn(boost::format(Y("matroska_reader: The height for track %1% was not set.\n")) % t->tnum);
    return;
  }

  // This track seems to be ok.
  t->ok = 1;
}

bool
kax_reader_c::verify_dvb_subtitle_track(kax_track_t *t) {
  if (   !t->private_data
      || (t->private_data->get_size() < 4)
      || (t->private_data->get_size() > 5)) {
    mxwarn(boost::format(Y("matroska_reader: The CodecID for track %1% is '%2%', but the private codec data does not contain valid headers.\n")) % t->tnum % t->codec_id);
    return false;
  }

  if (t->private_data->get_size() == 4) {
    // The subtitling type byte is missing. Add it. From ETSI EN 300 468 table 26:
    // 0x10 = DVB subtitles (normal) with no monitor aspect ratio criticality

    t->private_data->resize(5);
    t->private_data->get_buffer()[4] = 0x10;
  }

  return true;
}

bool
kax_reader_c::verify_hdmv_textst_subtitle_track(kax_track_t *t) {
  if (!t->private_data || (t->private_data->get_size() < 4)) {
    mxwarn(boost::format(Y("matroska_reader: The CodecID for track %1% is '%2%', but the private codec data does not contain valid headers.\n")) % t->tnum % t->codec_id);
    return false;
  }

  // Older files created by MakeMKV have a different layout:
  // 1 byte language code
  // segment descriptor:
  //   1 byte segment type (dialog style segment)
  //   2 bytes segment size
  // x bytes dialog style segment data
  // 2 bytes frame count

  // Newer files will only contain the dialog style segment's
  // descriptor and data

  auto buf                 = t->private_data->get_buffer();
  auto old_style           = buf[0] && (buf[0] < 0x10);
  auto style_segment_start = old_style ? 1 : 0;
  auto style_segment_size  = get_uint16_be(&buf[style_segment_start + 1]);

  if (t->private_data->get_size() < static_cast<unsigned int>(3 + style_segment_size + (old_style ? 1 + 2 : 0))) {
    mxwarn(boost::format(Y("matroska_reader: The CodecID for track %1% is '%2%', but the private codec data does not contain valid headers.\n")) % t->codec_id);
    return false;
  }

  if (0 < style_segment_start)
    std::memmove(&buf[0], &buf[style_segment_start], style_segment_size);

  t->private_data->resize(style_segment_size + 3);

  return true;
}

bool
kax_reader_c::verify_kate_subtitle_track(kax_track_t *t) {
  if (t->private_data)
    return true;

  if (verbose)
    mxwarn(boost::format(Y("matroska_reader: The CodecID for track %1% is '%2%', but there was no private data found.\n")) % t->tnum % t->codec_id);

  return false;
}

bool
kax_reader_c::verify_vobsub_subtitle_track(kax_track_t *t) {
  if (t->private_data)
    return true;

  if (!g_identifying && verbose)
    mxwarn_fn(m_ti.m_fname,
              boost::format("%1% %2%\n")
              % (boost::format(Y("The VobSub subtitle track %1% does not contain its index in the CodecPrivate element.")) % t->tnum)
              % Y("A default index and with it default settings for the width, height and color palette will be used instead."));

  t->private_data = memory_c::clone(mtx::vobsub::create_default_index(720, 576, {}));

  return true;
}

void
kax_reader_c::verify_subtitle_track(kax_track_t *t) {
  auto is_ok = true;
  t->codec   = codec_c::look_up(t->codec_id);

  if (t->codec.is(codec_c::type_e::S_VOBSUB))
    is_ok = verify_vobsub_subtitle_track(t);

  else if (t->codec.is(codec_c::type_e::S_DVBSUB))
    is_ok = verify_dvb_subtitle_track(t);

  else if (t->codec.is(codec_c::type_e::S_KATE))
    is_ok = verify_kate_subtitle_track(t);

  else if (t->codec.is(codec_c::type_e::S_HDMV_TEXTST))
    is_ok = verify_hdmv_textst_subtitle_track(t);

  t->ok = is_ok ? 1 : 0;
}

void
kax_reader_c::verify_button_track(kax_track_t *t) {
  t->codec = codec_c::look_up(t->codec_id);

  if (!t->codec.is(codec_c::type_e::B_VOBBTN)) {
    if (verbose)
      mxwarn(boost::format(Y("matroska_reader: The CodecID '%1%' for track %2% is unknown.\n")) % t->codec_id % t->tnum);
    return;
  }

  t->ok = 1;
}

void
kax_reader_c::verify_tracks() {
  size_t tnum;
  kax_track_t *t;

  for (tnum = 0; tnum < m_tracks.size(); tnum++) {
    t = m_tracks[tnum].get();

    t->ok = t->content_decoder.is_ok();

    if (!t->ok)
      continue;
    t->ok = 0;

    if (t->private_data) {
      t->content_decoder.reverse(t->private_data, CONTENT_ENCODING_SCOPE_CODECPRIVATE);
    }

    switch (t->type) {
      case 'v': verify_video_track(t);    break;
      case 'a': verify_audio_track(t);    break;
      case 's': verify_subtitle_track(t); break;
      case 'b': verify_button_track(t);   break;

      default:                  // unknown track type!? error in demuxer...
        if (verbose)
          mxwarn(boost::format(Y("matroska_reader: unknown demultiplexer type for track %1%: '%2%'\n")) % t->tnum % t->type);
        continue;
    }

    if (t->ok && (1 < verbose))
      mxinfo(boost::format(Y("matroska_reader: Track %1% seems to be ok.\n")) % t->tnum);
  }
}

bool
kax_reader_c::has_deferred_element_been_processed(kax_reader_c::deferred_l1_type_e type,
                                                  int64_t position) {
  for (auto test_position : m_handled_l1_positions[type])
    if (position == test_position)
      return true;

  m_handled_l1_positions[type].push_back(position);

  return false;
}

void
kax_reader_c::handle_attachments(mm_io_c *io,
                                 EbmlElement *l0,
                                 int64_t pos) {
  if (has_deferred_element_been_processed(dl1t_attachments, pos))
    return;

  io->save_pos(pos);
  mtx::at_scope_exit_c restore([io]() { io->restore_pos(); });

  int upper_lvl_el = 0;
  std::shared_ptr<EbmlElement> l1(m_es->FindNextElement(EBML_CONTEXT(l0), upper_lvl_el, 0xFFFFFFFFL, true));
  KaxAttachments *atts = dynamic_cast<KaxAttachments *>(l1.get());

  if (!atts)
    return;

  EbmlElement *element_found = nullptr;
  upper_lvl_el               = 0;

  atts->Read(*m_es, EBML_CLASS_CONTEXT(KaxAttachments), upper_lvl_el, element_found, true);

  for (auto l1_att : *atts) {
    auto att = dynamic_cast<KaxAttached *>(l1_att);
    if (!att)
      continue;

    ++m_attachment_id;

    auto fdata = FindChild<KaxFileData>(att);
    if (!fdata)
      continue;

    auto matt         = std::make_shared<attachment_t>();
    matt->name        = to_utf8(FindChildValue<KaxFileName>(att));
    matt->description = to_utf8(FindChildValue<KaxFileDescription>(att));
    matt->mime_type   = FindChildValue<KaxMimeType>(att);
    matt->id          = FindChildValue<KaxFileUID>(att);
    matt->data        = memory_c::clone(static_cast<unsigned char *>(fdata->GetBuffer()), fdata->GetSize());

    auto attach_mode  = attachment_requested(m_attachment_id);

    if (   !matt->data->get_size()
        || matt->mime_type.empty()
        || matt->name.empty()
        || (ATTACH_MODE_SKIP == attach_mode))
      continue;

    matt->ui_id          = m_attachment_id;
    matt->to_all_files   = ATTACH_MODE_TO_ALL_FILES == attach_mode;
    matt->source_file    = m_ti.m_fname;

    add_attachment(matt);
  }
}

void
kax_reader_c::handle_chapters(mm_io_c *io,
                              EbmlElement *l0,
                              int64_t pos) {
  if (has_deferred_element_been_processed(dl1t_chapters, pos))
    return;

  io->save_pos(pos);
  mtx::at_scope_exit_c restore([io]() { io->restore_pos(); });

  int upper_lvl_el = 0;
  std::shared_ptr<EbmlElement> l1(m_es->FindNextElement(EBML_CONTEXT(l0), upper_lvl_el, 0xFFFFFFFFL, true));
  KaxChapters *tmp_chapters = dynamic_cast<KaxChapters *>(l1.get());

  if (!tmp_chapters)
    return;

  EbmlElement *l2 = nullptr;
  upper_lvl_el    = 0;

  tmp_chapters->Read(*m_es, EBML_CLASS_CONTEXT(KaxChapters), upper_lvl_el, l2, true);

  if (m_regenerate_chapter_uids)
    mtx::chapters::regenerate_uids(*tmp_chapters);

  if (!m_chapters)
    m_chapters = mtx::chapters::kax_cptr{new KaxChapters};

  m_chapters->GetElementList().insert(m_chapters->begin(), tmp_chapters->begin(), tmp_chapters->end());
  tmp_chapters->RemoveAll();
}

void
kax_reader_c::handle_tags(mm_io_c *io,
                          EbmlElement *l0,
                          int64_t pos) {
  if (has_deferred_element_been_processed(dl1t_tags, pos))
    return;

  io->save_pos(pos);
  mtx::at_scope_exit_c restore([io]() { io->restore_pos(); });

  int upper_lvl_el = 0;
  std::shared_ptr<EbmlElement> l1(m_es->FindNextElement(EBML_CONTEXT(l0), upper_lvl_el, 0xFFFFFFFFL, true));
  KaxTags *tags = dynamic_cast<KaxTags *>(l1.get());

  if (!tags)
    return;

  EbmlElement *l2 = nullptr;
  upper_lvl_el    = 0;

  tags->Read(*m_es, EBML_CLASS_CONTEXT(KaxTags), upper_lvl_el, l2, true);

  while (tags->ListSize() > 0) {
    if (!Is<KaxTag>((*tags)[0])) {
      delete (*tags)[0];
      tags->Remove(0);
      continue;
    }

    bool delete_tag       = true;
    bool is_global        = true;
    KaxTag *tag           = static_cast<KaxTag *>((*tags)[0]);
    KaxTagTargets *target = FindChild<KaxTagTargets>(tag);

    if (target) {
      KaxTagTrackUID *tuid = FindChild<KaxTagTrackUID>(target);

      if (tuid) {
        is_global          = false;
        kax_track_t *track = find_track_by_uid(tuid->GetValue());

        if (track) {
          bool contains_tag = false;

          size_t i;
          for (i = 0; i < tag->ListSize(); i++)
            if (dynamic_cast<KaxTagSimple *>((*tag)[i])) {
              contains_tag = true;
              break;
            }

          if (contains_tag) {
            if (!track->tags)
              track->tags = new KaxTags;
            track->tags->PushElement(*tag);

            delete_tag = false;
          }
        }
      }
    }

    if (is_global) {
      if (!m_tags)
        m_tags = std::shared_ptr<KaxTags>(new KaxTags);
      m_tags->PushElement(*tag);

    } else if (delete_tag)
      delete tag;

    tags->Remove(0);
  }
}

void
kax_reader_c::discard_track_statistics_tags() {
  for (auto const &track : m_tracks)
    track->discard_track_statistics_tags();
}

void
kax_reader_c::read_headers_info(mm_io_c *io,
                                EbmlElement *l0,
                                int64_t pos) {
  // General info about this Matroska file
  if (has_deferred_element_been_processed(dl1t_info, pos))
    return;

  io->save_pos(pos);
  mtx::at_scope_exit_c restore([io]() { io->restore_pos(); });

  int upper_lvl_el = 0;
  std::shared_ptr<EbmlElement> l1(m_es->FindNextElement(EBML_CONTEXT(l0), upper_lvl_el, 0xFFFFFFFFL, true));
  KaxInfo *info = dynamic_cast<KaxInfo *>(l1.get());

  if (!info)
    return;

  EbmlElement *l2 = nullptr;
  upper_lvl_el    = 0;

  info->Read(*m_es, EBML_CLASS_CONTEXT(KaxInfo), upper_lvl_el, l2, true);

  m_tc_scale          = FindChildValue<KaxTimecodeScale, uint64_t>(info, 1000000);
  m_segment_duration  = std::llround(FindChildValue<KaxDuration>(info) * m_tc_scale);
  m_title             = to_utf8(FindChildValue<KaxTitle>(info));
  auto muxing_date    = FindChild<KaxDateUTC>(info);
  if (muxing_date)
    m_muxing_date_epoch = muxing_date->GetEpochDate();

  m_in_file->set_timestamp_scale(m_tc_scale);

  // Let's try to parse the "writing application" string. This usually
  // contains the name and version number of the application used for
  // creating this Matroska file. Examples are:
  //
  // mkvmerge v0.6.6
  // mkvmerge v0.9.6 ('Every Little Kiss') built on Oct  7 2004 18:37:49
  // VirtualDubMod 1.5.4.1 (build 2178/release)
  // AVI-Mux GUI 1.16.8 MPEG test build 1, Aug 24 2004  12:42:57
  // HandBrake 0.10.2 2015060900
  //
  // The idea is to first replace known application names that contain
  // spaces with one that doesn't. Then split the whole std::string up on
  // spaces into at most three parts. If the result is at least two parts
  // long then try to parse the version number from the second and
  // store a lower case version of the first as the application's name.
  auto km_writing_app = FindChild<KaxWritingApp>(info);
  if (km_writing_app) {
    read_headers_info_writing_app(km_writing_app);

    // Workaround: HandBrake and other tools always assign sequential
    // numbers starting at 1 to UID attributes. This is a problem when
    // appending two files created by such a tool that contain chapters
    // as both files contain chapters with the same UIDs and mkvmerge
    // thinks those should be merged. So ignore the chapter UIDs for
    // files that aren't created by known-good applications.
    if (!boost::regex_search(m_writing_app, boost::regex{"^(?:mkvmerge|no_variable_data)", boost::regex::perl | boost::regex::icase}))
      m_regenerate_chapter_uids = true;
  }

  auto km_muxing_app = FindChild<KaxMuxingApp>(info);
  if (km_muxing_app) {
    m_muxing_app = km_muxing_app->GetValueUTF8();

    // DirectShow Muxer workaround: Gabest's DirectShow muxer
    // writes wrong references (off by 1ms). So let the cluster
    // helper be a bit more imprecise in what it accepts when
    // looking for referenced packets.
    if (m_muxing_app == "DirectShow Matroska Muxer")
      m_reference_timestamp_tolerance = 1000000;
  }

  m_segment_uid          = FindChildValue<KaxSegmentUID>(info);
  m_next_segment_uid     = FindChildValue<KaxNextUID>(info);
  m_previous_segment_uid = FindChildValue<KaxPrevUID>(info);
}

void
kax_reader_c::read_headers_info_writing_app(KaxWritingApp *&km_writing_app) {
  size_t idx;

  std::string s = km_writing_app->GetValueUTF8();
  strip(s);

  m_raw_writing_app = s;

  if (balg::istarts_with(s, "avi-mux gui"))
    s.replace(0, strlen("avi-mux gui"), "avimuxgui");

  std::vector<std::string> parts = split(s.c_str(), " ", 3);
  if (parts.size() < 2) {
    m_writing_app = "";
    for (idx = 0; idx < s.size(); idx++)
      m_writing_app += tolower(s[idx]);
    m_writing_app_ver = -1;

  } else {

    m_writing_app = "";
    for (idx = 0; idx < parts[0].size(); idx++)
      m_writing_app += tolower(parts[0][idx]);
    s = "";

    for (idx = 0; idx < parts[1].size(); idx++)
      if (isdigit(parts[1][idx]) || (parts[1][idx] == '.'))
        s += parts[1][idx];

    std::vector<std::string> ver_parts = split(s.c_str(), ".");
    for (idx = ver_parts.size(); idx < 4; idx++)
      ver_parts.push_back("0");

    bool failed     = false;
    m_writing_app_ver = 0;

    for (idx = 0; idx < 4; idx++) {
      int num;

      if (!parse_number(ver_parts[idx], num) || (0 > num) || (255 < num)) {
        failed = true;
        break;
      }
      m_writing_app_ver <<= 8;
      m_writing_app_ver  |= num;
    }
    if (failed)
      m_writing_app_ver = -1;
  }
}

void
kax_reader_c::read_headers_track_audio(kax_track_t *track,
                                       KaxTrackAudio *ktaudio) {
  track->a_sfreq    = FindChildValue<KaxAudioSamplingFreq>(ktaudio, 8000.0);
  track->a_osfreq   = FindChildValue<KaxAudioOutputSamplingFreq>(ktaudio);
  track->a_channels = FindChildValue<KaxAudioChannels>(ktaudio, 1);
  track->a_bps      = FindChildValue<KaxAudioBitDepth>(ktaudio);
}

void
kax_reader_c::read_headers_track_video(kax_track_t *track,
                                       KaxTrackVideo *ktvideo) {
  track->v_width        = FindChildValue<KaxVideoPixelWidth>(ktvideo);
  track->v_height       = FindChildValue<KaxVideoPixelHeight>(ktvideo);
  track->v_dwidth       = FindChildValue<KaxVideoDisplayWidth>(ktvideo,  track->v_width);
  track->v_dheight      = FindChildValue<KaxVideoDisplayHeight>(ktvideo, track->v_height);

  track->v_pcleft       = FindChildValue<KaxVideoPixelCropLeft>(ktvideo);
  track->v_pcright      = FindChildValue<KaxVideoPixelCropRight>(ktvideo);
  track->v_pctop        = FindChildValue<KaxVideoPixelCropTop>(ktvideo);
  track->v_pcbottom     = FindChildValue<KaxVideoPixelCropBottom>(ktvideo);

  auto colour           = FindChild<KaxVideoColour>(*ktvideo);

  if (colour) {
    track->v_colour_matrix         = FindChildValue<KaxVideoColourMatrix>(colour, -1);
    track->v_bits_per_channel      = FindChildValue<KaxVideoBitsPerChannel>(colour, -1);
    track->v_chroma_subsample.hori = FindChildValue<KaxVideoChromaSubsampHorz>(colour, -1);
    track->v_chroma_subsample.vert = FindChildValue<KaxVideoChromaSubsampVert>(colour, -1);
    track->v_cb_subsample.hori     = FindChildValue<KaxVideoCbSubsampHorz>(colour, -1);
    track->v_cb_subsample.vert     = FindChildValue<KaxVideoCbSubsampVert>(colour, -1);
    track->v_chroma_siting.hori    = FindChildValue<KaxVideoChromaSitHorz>(colour, -1);
    track->v_chroma_siting.vert    = FindChildValue<KaxVideoChromaSitVert>(colour, -1);
    track->v_colour_range          = FindChildValue<KaxVideoColourRange>(colour, -1);
    track->v_transfer_character    = FindChildValue<KaxVideoColourTransferCharacter>(colour, -1);
    track->v_colour_primaries      = FindChildValue<KaxVideoColourPrimaries>(colour, -1);
    track->v_max_cll               = FindChildValue<KaxVideoColourMaxCLL>(colour, -1);
    track->v_max_fall              = FindChildValue<KaxVideoColourMaxFALL>(colour, -1);

    auto colour_meta               = FindChild<KaxVideoColourMasterMeta>(*colour);

    if (colour_meta) {
      track->v_chroma_coordinates.red_x   = FindChildValue<KaxVideoRChromaX>(colour_meta, -1.0);
      track->v_chroma_coordinates.red_y   = FindChildValue<KaxVideoRChromaY>(colour_meta, -1.0);
      track->v_chroma_coordinates.green_x = FindChildValue<KaxVideoGChromaX>(colour_meta, -1.0);
      track->v_chroma_coordinates.green_y = FindChildValue<KaxVideoGChromaY>(colour_meta, -1.0);
      track->v_chroma_coordinates.blue_x  = FindChildValue<KaxVideoBChromaX>(colour_meta, -1.0);
      track->v_chroma_coordinates.blue_y  = FindChildValue<KaxVideoBChromaY>(colour_meta, -1.0);
      track->v_white_colour_coordinates.x = FindChildValue<KaxVideoWhitePointChromaX>(colour_meta, -1.0);
      track->v_white_colour_coordinates.y = FindChildValue<KaxVideoWhitePointChromaY>(colour_meta, -1.0);
      track->v_max_luminance              = FindChildValue<KaxVideoLuminanceMax>(colour_meta, -1.0);
      track->v_min_luminance              = FindChildValue<KaxVideoLuminanceMin>(colour_meta, -1.0);
    }
  }

  auto projection = FindChild<KaxVideoProjection>(*ktvideo);

  if (projection) {
    track->v_projection_type       = FindOptionalChildValue<KaxVideoProjectionType>(projection);
    track->v_projection_pose_yaw   = FindOptionalChildValue<KaxVideoProjectionPoseYaw>(projection);
    track->v_projection_pose_pitch = FindOptionalChildValue<KaxVideoProjectionPosePitch>(projection);
    track->v_projection_pose_roll  = FindOptionalChildValue<KaxVideoProjectionPoseRoll>(projection);

    auto kprojection_private = FindChild<KaxVideoProjectionPrivate>(projection);
    if (kprojection_private)
      track->v_projection_private = memory_c::clone(kprojection_private->GetBuffer(), kprojection_private->GetSize());
  }

  track->v_field_order  = FindChildValue<KaxVideoFieldOrder>(ktvideo, -1);
  track->v_stereo_mode  = FindChildValue<KaxVideoStereoMode, stereo_mode_c::mode>(ktvideo, stereo_mode_c::unspecified);

  // For older files.
  track->v_frate        = FindChildValue<KaxVideoFrameRate>(ktvideo, track->v_frate);

#if MATROSKA_VERSION >= 2
  track->v_display_unit = FindChildValue<KaxVideoDisplayUnit>(ktvideo);
#endif

  if (!track->v_width)
    mxerror(Y("matroska_reader: Pixel width is missing.\n"));
  if (!track->v_height)
    mxerror(Y("matroska_reader: Pixel height is missing.\n"));

  track->fix_display_dimension_parameters();
}

void
kax_reader_c::read_headers_tracks(mm_io_c *io,
                                  EbmlElement *l0,
                                  int64_t position) {
  // Yep, we've found our KaxTracks element. Now find all m_tracks
  // contained in this segment.
  if (has_deferred_element_been_processed(dl1t_tracks, position))
    return;

  int upper_lvl_el = 0;
  io->save_pos(position);
  EbmlElement *l1 = m_es->FindNextElement(EBML_CONTEXT(l0), upper_lvl_el, 0xFFFFFFFFL, true);

  if (!l1 || !Is<KaxTracks>(l1)) {
    delete l1;
    io->restore_pos();

    return;
  }

  EbmlElement *l2 = nullptr;
  upper_lvl_el    = 0;
  l1->Read(*m_es, EBML_CLASS_CONTEXT(KaxTracks), upper_lvl_el, l2, true);

  KaxTrackEntry *ktentry = FindChild<KaxTrackEntry>(l1);
  while (ktentry) {
    // We actually found a track entry :) We're happy now.
    auto track = std::make_shared<kax_track_t>();
    track->tnum = m_tracks.size();

    auto ktnum = FindChild<KaxTrackNumber>(ktentry);
    if (!ktnum)
      mxerror(Y("matroska_reader: A track is missing its track number.\n"));

    track->track_number = ktnum->GetValue();
    if (find_track_by_num(track->track_number, track.get())) {
      ktentry = FindNextChild(static_cast<EbmlMaster &>(*l1), *ktentry);
      continue;
    }

    auto ktuid = FindChild<KaxTrackUID>(ktentry);
    if (!ktuid)
      mxwarn_fn(m_ti.m_fname,
                boost::format(Y("Track %1% is missing its track UID element which is required to be present by the Matroska specification. If the file contains tags then those tags might be broken.\n"))
                % track->tnum);
    else
      track->track_uid = ktuid->GetValue();

    auto kttype = FindChild<KaxTrackType>(ktentry);
    if (!kttype)
      mxerror(Y("matroska_reader: Track type was not found.\n"));
    unsigned char track_type = kttype->GetValue();
    track->type              = track_type == track_audio    ? 'a'
                             : track_type == track_video    ? 'v'
                             : track_type == track_subtitle ? 's'
                             :                                '?';

    auto ktaudio = FindChild<KaxTrackAudio>(ktentry);
    if (ktaudio)
      read_headers_track_audio(track.get(), ktaudio);

    auto ktvideo = FindChild<KaxTrackVideo>(ktentry);
    if (ktvideo)
      read_headers_track_video(track.get(), ktvideo);

    auto kcodecpriv = FindChild<KaxCodecPrivate>(ktentry);
    if (kcodecpriv) {
      track->private_data = memory_c::clone(kcodecpriv->GetBuffer(), kcodecpriv->GetSize());
    }

    track->codec_id         = FindChildValue<KaxCodecID>(ktentry);
    track->track_name       = to_utf8(FindChildValue<KaxTrackName>(ktentry));
    track->language         = FindChildValue<KaxTrackLanguage, std::string>(ktentry, "eng");
    track->default_duration = FindChildValue<KaxTrackDefaultDuration>(ktentry, track->default_duration);
    track->default_track    = FindChildValue<KaxTrackFlagDefault, bool>(ktentry, true);
    track->forced_track     = FindChildValue<KaxTrackFlagForced>(ktentry);
    track->enabled_track    = FindChildValue<KaxTrackFlagEnabled, bool>(ktentry, true);
    track->lacing_flag      = FindChildValue<KaxTrackFlagLacing>(ktentry);
    track->max_blockadd_id  = FindChildValue<KaxMaxBlockAdditionID>(ktentry);

    auto kax_seek_pre_roll  = FindChild<KaxSeekPreRoll>(ktentry);
    auto kax_codec_delay    = FindChild<KaxCodecDelay>(ktentry);

    if (kax_seek_pre_roll)
      track->seek_pre_roll = timestamp_c::ns(kax_seek_pre_roll->GetValue());
    if (kax_codec_delay)
      track->codec_delay   = timestamp_c::ns(kax_codec_delay->GetValue());

    if (track->codec_id.empty())
      mxerror(Y("matroska_reader: The CodecID is missing.\n"));

    if (!track->language.empty()) {
      int index = map_to_iso639_2_code(track->language.c_str());
      if (-1 != index)
        track->language = g_iso639_languages[index].iso639_2_code;
      else
        track->language.clear();
    }

    // The variable can be empty in two cases, both of which are a
    // violation of the specification:
    //
    // 1. The element is present but set to a string of length 0.
    // 2. The element is present and set to a value that isn't a valid
    //    ISO 639-2 language code.
    //
    // The closest code that's semantically the closest to such a
    // situation is probably "und" = "undetermined".
    if (track->language.empty())
      track->language = "und";

    if (0 != track->default_duration)
      track->v_frate = 1000000000.0 / track->default_duration;

    track->content_decoder.initialize(*ktentry);
    m_tracks.push_back(track);

    ktentry = FindNextChild(static_cast<EbmlMaster &>(*l1), *ktentry);
  } // while (ktentry)

  delete l1;
  io->restore_pos();
}

void
kax_reader_c::handle_seek_head(mm_io_c *io,
                               EbmlElement *l0,
                               int64_t pos) {
  if (has_deferred_element_been_processed(dl1t_seek_head, pos))
    return;

  std::vector<int64_t> next_seek_head_positions;
  mtx::at_scope_exit_c restore([io]() { io->restore_pos(); });

  try {
    io->save_pos(pos);

    int upper_lvl_el = 0;
    std::shared_ptr<EbmlElement> l1(m_es->FindNextElement(EBML_CONTEXT(l0), upper_lvl_el, 0xFFFFFFFFL, true));
    auto *seek_head = dynamic_cast<KaxSeekHead *>(l1.get());

    if (!seek_head)
      return;

    EbmlElement *l2 = nullptr;
    upper_lvl_el    = 0;

    seek_head->Read(*m_es, EBML_CLASS_CONTEXT(KaxSeekHead), upper_lvl_el, l2, true);

    for (auto l2 : *seek_head) {
      if (!Is<KaxSeek>(l2))
        continue;

      KaxSeek &seek = *static_cast<KaxSeek *>(l2);
      int64_t pos   = FindChildValue<KaxSeekPosition, int64_t>(seek, -1);

      if (-1 == pos)
        continue;

      auto *k_id = FindChild<KaxSeekID>(seek);
      if (!k_id)
        continue;

      EbmlId id(k_id->GetBuffer(), k_id->GetSize());

      deferred_l1_type_e type = Is<KaxAttachments>(id) ? dl1t_attachments
        :                       Is<KaxChapters>(id)    ? dl1t_chapters
        :                       Is<KaxTags>(id)        ? dl1t_tags
        :                       Is<KaxTracks>(id)      ? dl1t_tracks
        :                       Is<KaxSeekHead>(id)    ? dl1t_seek_head
        :                       Is<KaxInfo>(id)        ? dl1t_info
        :                                                dl1t_unknown;

      if (dl1t_unknown == type)
        continue;

      pos = static_cast<KaxSegment *>(l0)->GetGlobalPosition(pos);

      if (dl1t_seek_head == type)
        next_seek_head_positions.push_back(pos);
      else
        m_deferred_l1_positions[type].push_back(pos);
    }

  } catch (...) {
    return;
  }

  for (auto pos : next_seek_head_positions)
    handle_seek_head(io, l0, pos);
}

void
kax_reader_c::read_headers() {
  if (!read_headers_internal())
    throw mtx::input::header_parsing_x();

  determine_minimum_timestamps();
  determine_global_timestamp_offset_to_apply();

  show_demuxer_info();
}

void
kax_reader_c::find_level1_elements_via_analyzer() {
  try {
    auto start_pos = m_in->get_size() - std::min<int64_t>(m_in->get_size(), 5 * 1024 * 1024);
    auto analyzer  = std::make_shared<kax_analyzer_c>(m_in.get());
    auto ok        = analyzer
      ->set_parse_mode(kax_analyzer_c::parse_mode_full)
      .set_open_mode(MODE_READ)
      .set_parser_start_position(start_pos)
      .process();

    if (!ok)
      return;

    analyzer->with_elements(EBML_ID(KaxInfo),        [this](kax_analyzer_data_c const &data) { m_deferred_l1_positions[dl1t_info       ].push_back(data.m_pos); });
    analyzer->with_elements(EBML_ID(KaxTracks),      [this](kax_analyzer_data_c const &data) { m_deferred_l1_positions[dl1t_tracks     ].push_back(data.m_pos); });
    analyzer->with_elements(EBML_ID(KaxAttachments), [this](kax_analyzer_data_c const &data) { m_deferred_l1_positions[dl1t_attachments].push_back(data.m_pos); });
    analyzer->with_elements(EBML_ID(KaxChapters),    [this](kax_analyzer_data_c const &data) { m_deferred_l1_positions[dl1t_chapters   ].push_back(data.m_pos); });
    analyzer->with_elements(EBML_ID(KaxTags),        [this](kax_analyzer_data_c const &data) { m_deferred_l1_positions[dl1t_tags       ].push_back(data.m_pos); });

  } catch (...) {
  }
}

void
kax_reader_c::read_deferred_level1_elements(KaxSegment &segment) {
  for (auto position : m_deferred_l1_positions[dl1t_info])
    read_headers_info(m_in.get(), &segment, position);

  for (auto position : m_deferred_l1_positions[dl1t_tracks])
    read_headers_tracks(m_in.get(), &segment, position);

  if (!m_ti.m_attach_mode_list.none())
    for (auto position : m_deferred_l1_positions[dl1t_attachments])
      handle_attachments(m_in.get(), &segment, position);

  if (!m_ti.m_no_chapters)
    for (auto position : m_deferred_l1_positions[dl1t_chapters])
      handle_chapters(m_in.get(), &segment, position);

  for (auto position : m_deferred_l1_positions[dl1t_tags])
    handle_tags(m_in.get(), &segment, position);

  if (!hack_engaged(ENGAGE_KEEP_TRACK_STATISTICS_TAGS))
    discard_track_statistics_tags();

  if (!m_ti.m_no_global_tags)
    process_global_tags();
}

bool
kax_reader_c::read_headers_internal() {
  // Elements for different levels

  auto cluster = std::shared_ptr<KaxCluster>{};
  try {
    m_es      = std::shared_ptr<EbmlStream>(new EbmlStream(*m_in));
    m_in_file = std::make_shared<kax_file_c>(*m_in);

    m_in_file->enable_reporting(!g_identifying);

    // Find the EbmlHead element. Must be the first one.
    EbmlElement *l0 = m_es->FindNextID(EBML_INFO(EbmlHead), 0xFFFFFFFFFFFFFFFFLL);
    if (!l0) {
      mxwarn(Y("matroska_reader: no EBML head found.\n"));
      return false;
    }

    // Don't verify its data for now.
    l0->SkipData(*m_es, EBML_CONTEXT(l0));
    delete l0;

    // Next element must be a segment
    l0 = m_es->FindNextID(EBML_INFO(KaxSegment), 0xFFFFFFFFFFFFFFFFLL);
    std::shared_ptr<EbmlElement> l0_cptr(l0);
    if (!l0) {
      if (verbose)
        mxwarn(Y("matroska_reader: No segment found.\n"));
      return false;
    }
    if (!Is<KaxSegment>(l0)) {
      if (verbose)
        mxwarn(Y("matroska_reader: No segment found.\n"));
      return false;
    }

    m_in_file->set_segment_end(*l0);

    // We've got our segment, so let's find the m_tracks
    m_tc_scale = TIMESTAMP_SCALE;

    while (m_in->getFilePointer() < m_in_file->get_segment_end()) {
      auto l1 = ebml_element_cptr{ m_in_file->read_next_level1_element() };
      if (!l1)
        break;

      if (Is<KaxInfo>(*l1))
        m_deferred_l1_positions[dl1t_info].push_back(l1->GetElementPosition());

      else if (Is<KaxTracks>(*l1))
        m_deferred_l1_positions[dl1t_tracks].push_back(l1->GetElementPosition());

      else if (Is<KaxAttachments>(*l1))
        m_deferred_l1_positions[dl1t_attachments].push_back(l1->GetElementPosition());

      else if (Is<KaxChapters>(*l1))
        m_deferred_l1_positions[dl1t_chapters].push_back(l1->GetElementPosition());

      else if (Is<KaxTags>(*l1))
        m_deferred_l1_positions[dl1t_tags].push_back(l1->GetElementPosition());

      else if (Is<KaxSeekHead>(*l1))
        handle_seek_head(m_in.get(), l0, l1->GetElementPosition());

      else if (Is<KaxCluster>(*l1))
        cluster = std::static_pointer_cast<KaxCluster>(l1);

      else
        l1->SkipData(*m_es, EBML_CONTEXT(l1));

      if (cluster)              // we've found the first cluster, so get out
        break;

      if (!in_parent(l0))
        break;

      l1->SkipData(*m_es, EBML_CONTEXT(l1));

    } // while (l1)

    if (m_handled_l1_positions[dl1t_seek_head].empty() || debugging_c::requested("kax_reader_use_analyzer"))
      find_level1_elements_via_analyzer();

    read_deferred_level1_elements(static_cast<KaxSegment &>(*l0));

  } catch (...) {
    mxwarn(boost::format("%1% %2% %3%\n")
           % (boost::format(Y("%1%: an unknown exception occurred.")) % "kax_reader_c::read_headers_internal()")
           % Y("This usually indicates a damaged file structure.") % Y("The file will not be processed further."));
  }

  auto cluster_pos = cluster ? cluster->GetElementPosition() : m_in->get_size();
  m_in->setFilePointer(cluster_pos, seek_beginning);

  verify_tracks();

  m_in->setFilePointer(cluster_pos, seek_beginning);

  return true;
}

void
kax_reader_c::process_global_tags() {
  if (!m_tags || g_identifying)
    return;

  for (auto tag : *m_tags)
    add_tags(static_cast<KaxTag *>(tag));

  m_tags->RemoveAll();
}

void
kax_reader_c::init_passthrough_packetizer(kax_track_t *t,
                                          track_info_c &nti) {
  passthrough_packetizer_c *ptzr;

  mxinfo_tid(m_ti.m_fname, t->tnum, boost::format(Y("Using the generic output module for track type '%1%'.\n")) % MAP_TRACK_TYPE_STRING(t->type));

  ptzr                      = new passthrough_packetizer_c(this, nti);
  t->ptzr                   = add_packetizer(ptzr);
  t->ptzr_ptr               = ptzr;
  t->passthrough            = true;
  m_ptzr_to_track_map[ptzr] = t;

  ptzr->set_track_type(MAP_TRACK_TYPE(t->type));
  ptzr->set_codec_id(t->codec_id);
  ptzr->set_codec_private(t->private_data);

  if (0.0 < t->v_frate)
    ptzr->set_track_default_duration(1000000000.0 / t->v_frate);
  if (t->seek_pre_roll.valid())
    ptzr->set_track_seek_pre_roll(t->seek_pre_roll);

  if ('v' == t->type) {
    ptzr->set_video_pixel_width(t->v_width);
    ptzr->set_video_pixel_height(t->v_height);

    t->handle_packetizer_display_dimensions();
    t->handle_packetizer_pixel_cropping();
    t->handle_packetizer_colour();
    t->handle_packetizer_field_order();
    t->handle_packetizer_stereo_mode();

    if (CUE_STRATEGY_UNSPECIFIED == ptzr->get_cue_creation())
      ptzr->set_cue_creation(CUE_STRATEGY_IFRAMES);

  } else if ('a' == t->type) {
    ptzr->set_audio_sampling_freq(t->a_sfreq);
    ptzr->set_audio_channels(t->a_channels);
    if (0 != t->a_bps)
      ptzr->set_audio_bit_depth(t->a_bps);
    if (0.0 != t->a_osfreq)
      ptzr->set_audio_output_sampling_freq(t->a_osfreq);

  } else {
    // Nothing to do for subs, I guess.
  }

}

void
kax_reader_c::set_packetizer_headers(kax_track_t *t) {
  if (m_appending)
    return;

  if (t->default_track)
    PTZR(t->ptzr)->set_as_default_track(t->type == 'v' ? DEFTRACK_TYPE_VIDEO : t->type == 'a' ? DEFTRACK_TYPE_AUDIO : DEFTRACK_TYPE_SUBS, DEFAULT_TRACK_PRIORITY_FROM_SOURCE);

  else if (!boost::logic::indeterminate(t->default_track) && boost::logic::indeterminate(PTZR(t->ptzr)->m_ti.m_default_track))
    PTZR(t->ptzr)->m_ti.m_default_track = false;

  if (t->forced_track && boost::logic::indeterminate(PTZR(t->ptzr)->m_ti.m_forced_track))
    PTZR(t->ptzr)->set_track_forced_flag(true);

  if (boost::logic::indeterminate(PTZR(t->ptzr)->m_ti.m_enabled_track))
    PTZR(t->ptzr)->set_track_enabled_flag(t->enabled_track);

  if ((0 != t->track_uid) && !PTZR(t->ptzr)->set_uid(t->track_uid))
    mxwarn(boost::format(Y("matroska_reader: Could not keep the track UID %1% because it is already allocated for the new file.\n")) % t->track_uid);
}

void
kax_reader_c::create_video_packetizer(kax_track_t *t,
                                      track_info_c &nti) {
  if (t->codec.is(codec_c::type_e::V_MPEG4_P10) && t->ms_compat && !hack_engaged(ENGAGE_ALLOW_AVC_IN_VFW_MODE))
    create_avc_es_video_packetizer(t, nti);

  else if (t->codec.is(codec_c::type_e::V_MPEG12)) {
    int version = t->codec_id[6] - '0';
    set_track_packetizer(t, new mpeg1_2_video_packetizer_c(this, nti, version, t->v_frate, t->v_width, t->v_height, t->v_dwidth, t->v_dheight, true));
    show_packetizer_info(t->tnum, t->ptzr_ptr);

  } else if (t->codec.is(codec_c::type_e::V_MPEGH_P2))
    create_hevc_video_packetizer(t, nti);

  else if (t->codec.is(codec_c::type_e::V_MPEG4_P2)) {
    bool is_native = IS_MPEG4_L2_CODECID(t->codec_id);
    set_track_packetizer(t, new mpeg4_p2_video_packetizer_c(this, nti, t->v_frate, t->v_width, t->v_height, is_native));
    show_packetizer_info(t->tnum, t->ptzr_ptr);

  } else if (t->codec.is(codec_c::type_e::V_MPEG4_P10))
    create_avc_video_packetizer(t, nti);

  else if (t->codec.is(codec_c::type_e::V_THEORA)) {
    set_track_packetizer(t, new theora_video_packetizer_c(this, nti, t->v_frate, t->v_width, t->v_height));
    show_packetizer_info(t->tnum, t->ptzr_ptr);

  } else if (t->codec.is(codec_c::type_e::V_DIRAC)) {
    set_track_packetizer(t, new dirac_video_packetizer_c(this, nti));
    show_packetizer_info(t->tnum, t->ptzr_ptr);

  } else if (t->codec.is(codec_c::type_e::V_VP8) || t->codec.is(codec_c::type_e::V_VP9)) {
    set_track_packetizer(t, new vpx_video_packetizer_c(this, nti, t->codec.get_type()));
    show_packetizer_info(t->tnum, t->ptzr_ptr);
    t->handle_packetizer_pixel_dimensions();
    t->handle_packetizer_default_duration();

  } else if (t->codec.is(codec_c::type_e::V_VC1))
    create_vc1_video_packetizer(t, nti);

  else if (t->ms_compat) {
    set_track_packetizer(t, new video_for_windows_packetizer_c(this, nti, t->v_frate, t->v_width, t->v_height));
    show_packetizer_info(t->tnum, t->ptzr_ptr);

  } else {
    set_track_packetizer(t, new generic_video_packetizer_c(this, nti, t->codec_id.c_str(), t->v_frate, t->v_width, t->v_height));
    show_packetizer_info(t->tnum, t->ptzr_ptr);
  }

  t->handle_packetizer_display_dimensions();
  t->handle_packetizer_pixel_cropping();
  t->handle_packetizer_colour();
  t->handle_packetizer_field_order();
  t->handle_packetizer_stereo_mode();
  t->handle_packetizer_codec_delay();
}

void
kax_reader_c::create_aac_audio_packetizer(kax_track_t *t,
                                          track_info_c &nti) {
  // A_AAC/MPEG2/MAIN
  // 0123456789012345
  mtx::aac::audio_config_t audio_config{};

  audio_config.sample_rate = t->a_sfreq;
  audio_config.channels    = t->a_channels;
  int detected_profile     = AAC_PROFILE_MAIN;

  if (!t->ms_compat) {
    if (t->private_data && (2 <= t->private_data->get_size())) {
      auto parsed_audio_config = mtx::aac::parse_audio_specific_config(t->private_data->get_buffer(), t->private_data->get_size());
      if (!parsed_audio_config)
        mxerror_tid(m_ti.m_fname, t->tnum, Y("Malformed AAC codec initialization data found.\n"));

      audio_config           = *parsed_audio_config;
      detected_profile       = audio_config.profile;
      if (audio_config.sbr)
        audio_config.profile = AAC_PROFILE_SBR;

    } else {
      int id = 0, profile = 0;
      if (!mtx::aac::parse_codec_id(t->codec_id, id, profile))
        mxerror_tid(m_ti.m_fname, t->tnum, boost::format(Y("Malformed codec id '%1%'.\n")) % t->codec_id);
      audio_config.profile = profile;
    }

  } else {
    auto parsed_audio_config = mtx::aac::parse_audio_specific_config(t->private_data->get_buffer() + sizeof(alWAVEFORMATEX), t->private_data->get_size() - sizeof(alWAVEFORMATEX));
    if (!parsed_audio_config)
      mxerror_tid(m_ti.m_fname, t->tnum, Y("Malformed AAC codec initialization data found.\n"));

    audio_config           = *parsed_audio_config;
    detected_profile       = audio_config.profile;
    if (audio_config.sbr)
      audio_config.profile = AAC_PROFILE_SBR;
  }

  if ((mtx::includes(m_ti.m_all_aac_is_sbr, t->tnum) &&  m_ti.m_all_aac_is_sbr[t->tnum]) || (mtx::includes(m_ti.m_all_aac_is_sbr, -1) &&  m_ti.m_all_aac_is_sbr[-1]))
    audio_config.profile = AAC_PROFILE_SBR;

  if ((mtx::includes(m_ti.m_all_aac_is_sbr, t->tnum) && !m_ti.m_all_aac_is_sbr[t->tnum]) || (mtx::includes(m_ti.m_all_aac_is_sbr, -1) && !m_ti.m_all_aac_is_sbr[-1]))
    audio_config.profile = detected_profile;

  set_track_packetizer(t, new aac_packetizer_c(this, nti, audio_config, aac_packetizer_c::headerless));
  show_packetizer_info(t->tnum, t->ptzr_ptr);
}

void
kax_reader_c::create_ac3_audio_packetizer(kax_track_t *t,
                                          track_info_c &nti) {
  unsigned int bsid = t->codec_id == "A_AC3/BSID9"  ?  9
                    : t->codec_id == "A_AC3/BSID10" ? 10
                    : t->codec_id == MKV_A_EAC3     ? 16
                    :                                  0;

  set_track_packetizer(t, new ac3_packetizer_c(this, nti, t->a_sfreq, t->a_channels, bsid));
  show_packetizer_info(t->tnum, t->ptzr_ptr);
}

void
kax_reader_c::create_alac_audio_packetizer(kax_track_t *t,
                                          track_info_c &nti) {
  set_track_packetizer(t, new alac_packetizer_c(this, nti, t->private_data, t->a_sfreq, t->a_channels));
  show_packetizer_info(t->tnum, t->ptzr_ptr);
}

void
kax_reader_c::create_dts_audio_packetizer(kax_track_t *t,
                                          track_info_c &nti) {
  set_track_packetizer(t, new dts_packetizer_c(this, nti, t->dts_header));
  show_packetizer_info(t->tnum, t->ptzr_ptr);
}

#if defined(HAVE_FLAC_FORMAT_H)
void
kax_reader_c::create_flac_audio_packetizer(kax_track_t *t,
                                           track_info_c &nti) {
  nti.m_private_data.reset();

  unsigned int offset = t->ms_compat ? sizeof(alWAVEFORMATEX) : 0u;
  set_track_packetizer(t, new flac_packetizer_c(this, nti, t->private_data->get_buffer() + offset, t->private_data->get_size() - offset));

  show_packetizer_info(t->tnum, t->ptzr_ptr);
}

#endif  // HAVE_FLAC_FORMAT_H

void
kax_reader_c::create_hevc_es_video_packetizer(kax_track_t *t,
                                              track_info_c &nti) {
  hevc_es_video_packetizer_c *ptzr = new hevc_es_video_packetizer_c(this, nti);
  set_track_packetizer(t, ptzr);

  ptzr->set_video_pixel_dimensions(t->v_width, t->v_height);

  show_packetizer_info(t->tnum, t->ptzr_ptr);
}

void
kax_reader_c::create_hevc_video_packetizer(kax_track_t *t,
                                           track_info_c &nti) {
  if (t->ms_compat || !nti.m_private_data || !nti.m_private_data->get_size()) {
    create_hevc_es_video_packetizer(t, nti);
    return;
  }

  set_track_packetizer(t, new hevc_video_packetizer_c(this, nti, t->v_frate, t->v_width, t->v_height));
  show_packetizer_info(t->tnum, t->ptzr_ptr);
}

void
kax_reader_c::create_mp3_audio_packetizer(kax_track_t *t,
                                          track_info_c &nti) {
  set_track_packetizer(t, new mp3_packetizer_c(this, nti, t->a_sfreq, t->a_channels, true));
  show_packetizer_info(t->tnum, t->ptzr_ptr);
}

void
kax_reader_c::create_opus_audio_packetizer(kax_track_t *t,
                                           track_info_c &nti) {
  set_track_packetizer(t, new opus_packetizer_c(this, nti));
  show_packetizer_info(t->tnum, t->ptzr_ptr);

  if (!m_opus_experimental_warning_shown && (t->codec_id == std::string{MKV_A_OPUS} + "/EXPERIMENTAL")) {
    mxwarn(boost::format(Y("'%1%': You're copying an Opus track that was written in experimental mode. "
                           "The resulting track will be written in final mode, but one detail cannot be recovered from a track written in experimental mode: the end trimming. "
                           "This means that a decoder might output a few samples more than originally intended. "
                           "You should re-multiplex from the original Opus file if possible.\n"))
           % m_ti.m_fname);
    m_opus_experimental_warning_shown = true;
  }
}

void
kax_reader_c::create_pcm_audio_packetizer(kax_track_t *t,
                                          track_info_c &nti) {
  auto type = t->codec_id == MKV_A_PCM_FLOAT ? pcm_packetizer_c::ieee_float
            : t->codec_id == MKV_A_PCM_BE    ? pcm_packetizer_c::big_endian_integer
            :                                  pcm_packetizer_c::little_endian_integer;
  set_track_packetizer(t, new pcm_packetizer_c(this, nti, t->a_sfreq, t->a_channels, t->a_bps, type));
  show_packetizer_info(t->tnum, t->ptzr_ptr);
}

void
kax_reader_c::create_truehd_audio_packetizer(kax_track_t *t,
                                             track_info_c &nti) {
  nti.m_private_data.reset();
  set_track_packetizer(t, new truehd_packetizer_c(this, nti, t->codec.is(codec_c::type_e::A_TRUEHD) ? truehd_frame_t::truehd : truehd_frame_t::mlp, t->a_sfreq, t->a_channels));
  show_packetizer_info(t->tnum, t->ptzr_ptr);
}

void
kax_reader_c::create_tta_audio_packetizer(kax_track_t *t,
                                          track_info_c &nti) {
  set_track_packetizer(t, new tta_packetizer_c(this, nti, t->a_channels, t->a_bps, t->a_sfreq));
  show_packetizer_info(t->tnum, t->ptzr_ptr);
}

void
kax_reader_c::create_vorbis_audio_packetizer(kax_track_t *t,
                                             track_info_c &nti) {
  set_track_packetizer(t, new vorbis_packetizer_c(this, nti, t->headers[0], t->header_sizes[0], t->headers[1], t->header_sizes[1], t->headers[2], t->header_sizes[2]));
  show_packetizer_info(t->tnum, t->ptzr_ptr);
}

void
kax_reader_c::create_wavpack_audio_packetizer(kax_track_t *t,
                                              track_info_c &nti) {
  nti.m_private_data.reset();

  wavpack_meta_t meta;
  meta.bits_per_sample = t->a_bps;
  meta.channel_count   = t->a_channels;
  meta.sample_rate     = t->a_sfreq;
  meta.has_correction  = t->max_blockadd_id != 0;

  if (0.0 < t->v_frate)
    meta.samples_per_block = t->a_sfreq / t->v_frate;

  set_track_packetizer(t, new wavpack_packetizer_c(this, nti, meta));

  show_packetizer_info(t->tnum, t->ptzr_ptr);
}

void
kax_reader_c::create_audio_packetizer(kax_track_t *t,
                                      track_info_c &nti) {
  if (t->codec.is(codec_c::type_e::A_PCM))
    create_pcm_audio_packetizer(t, nti);

  else if (t->codec.is(codec_c::type_e::A_MP2) || t->codec.is(codec_c::type_e::A_MP3))
    create_mp3_audio_packetizer(t, nti);

  else if (t->codec.is(codec_c::type_e::A_AC3))
    create_ac3_audio_packetizer(t, nti);

  else if (t->codec.is(codec_c::type_e::A_DTS))
    create_dts_audio_packetizer(t, nti);

  else if (t->codec.is(codec_c::type_e::A_VORBIS))
    create_vorbis_audio_packetizer(t, nti);

  else if (t->codec.is(codec_c::type_e::A_ALAC))
    create_alac_audio_packetizer(t, nti);

  else if (t->codec.is(codec_c::type_e::A_AAC))
    create_aac_audio_packetizer(t, nti);

#if defined(HAVE_FLAC_FORMAT_H)
  else if (t->codec.is(codec_c::type_e::A_FLAC))
    create_flac_audio_packetizer(t, nti);
#endif

  else if (t->codec.is(codec_c::type_e::A_OPUS))
    create_opus_audio_packetizer(t, nti);

  else if (t->codec.is(codec_c::type_e::A_TRUEHD))
    create_truehd_audio_packetizer(t, nti);

  else if (t->codec.is(codec_c::type_e::A_TTA))
    create_tta_audio_packetizer(t, nti);

  else if (t->codec.is(codec_c::type_e::A_WAVPACK4))
    create_wavpack_audio_packetizer(t, nti);

  else
    init_passthrough_packetizer(t, nti);

  t->handle_packetizer_output_sampling_freq();
  t->handle_packetizer_codec_delay();
}

void
kax_reader_c::create_subtitle_packetizer(kax_track_t *t,
                                         track_info_c &nti) {
  if (t->codec.is(codec_c::type_e::S_VOBSUB)) {
    set_track_packetizer(t, new vobsub_packetizer_c(this, nti));
    show_packetizer_info(t->tnum, t->ptzr_ptr);

    t->sub_type = 'v';

  } else if (t->codec.is(codec_c::type_e::S_DVBSUB)) {
    set_track_packetizer(t, new dvbsub_packetizer_c(this, nti, t->private_data));
    show_packetizer_info(t->tnum, t->ptzr_ptr);
    t->sub_type = 'p';

  } else if (t->codec.is(codec_c::type_e::S_WEBVTT)) {
    set_track_packetizer(t, new webvtt_packetizer_c(this, nti));
    show_packetizer_info(t->tnum, t->ptzr_ptr);

    t->sub_type = 't';

  } else if (balg::starts_with(t->codec_id, "S_TEXT") || (t->codec_id == "S_SSA") || (t->codec_id == "S_ASS")) {
    std::string new_codec_id = ((t->codec_id == "S_SSA") || (t->codec_id == "S_ASS")) ? std::string("S_TEXT/") + std::string(&t->codec_id[2]) : t->codec_id;

    set_track_packetizer(t, new textsubs_packetizer_c(this, nti, new_codec_id.c_str(), false, true));
    show_packetizer_info(t->tnum, t->ptzr_ptr);

    t->sub_type = 't';

  } else if (t->codec.is(codec_c::type_e::S_KATE)) {
    set_track_packetizer(t, new kate_packetizer_c(this, nti));
    show_packetizer_info(t->tnum, t->ptzr_ptr);
    t->sub_type = 'k';

  } else if (t->codec.is(codec_c::type_e::S_HDMV_PGS)) {
    set_track_packetizer(t, new hdmv_pgs_packetizer_c(this, nti));
    show_packetizer_info(t->tnum, t->ptzr_ptr);
    t->sub_type = 'p';

  } else if (t->codec.is(codec_c::type_e::S_HDMV_TEXTST)) {
    set_track_packetizer(t, new hdmv_textst_packetizer_c(this, nti, t->private_data));
    show_packetizer_info(t->tnum, t->ptzr_ptr);
    t->sub_type = 'p';

  } else
    init_passthrough_packetizer(t, nti);

}

void
kax_reader_c::create_button_packetizer(kax_track_t *t,
                                       track_info_c &nti) {
  if (!t->codec.is(codec_c::type_e::B_VOBBTN)) {
    init_passthrough_packetizer(t, nti);
    return;
  }

  nti.m_private_data.reset();
  t->sub_type = 'b';

  set_track_packetizer(t, new vobbtn_packetizer_c(this, nti, t->v_width, t->v_height));
  show_packetizer_info(t->tnum, t->ptzr_ptr);
}

void
kax_reader_c::create_packetizer(int64_t tid) {
  kax_track_t *t = m_tracks[tid].get();

  if ((-1 != t->ptzr) || !t->ok || !demuxing_requested(t->type, t->tnum, t->language))
    return;

  track_info_c nti(m_ti);
  nti.m_private_data = t->private_data ? t->private_data->clone() : memory_cptr{};
  nti.m_id           = t->tnum; // ID for this track.

  if (nti.m_language == "")
    nti.m_language   = t->language;
  if (nti.m_track_name == "")
    nti.m_track_name = t->track_name;
  if (t->tags && demuxing_requested('T', t->tnum))
    nti.m_tags       = clone(t->tags);

  if (hack_engaged(ENGAGE_FORCE_PASSTHROUGH_PACKETIZER)) {
    init_passthrough_packetizer(t, nti);
    set_packetizer_headers(t);

    return;
  }

  switch (t->type) {
    case 'v':
      create_video_packetizer(t, nti);
      break;

    case 'a':
      create_audio_packetizer(t, nti);
      break;

    case 's':
      create_subtitle_packetizer(t, nti);
      break;

    case 'b':
      create_button_packetizer(t, nti);
      break;

    default:
      mxerror_tid(m_ti.m_fname, t->tnum, Y("Unsupported track type for this track.\n"));
      break;
  }

  set_packetizer_headers(t);
  m_ptzr_to_track_map[ m_reader_packetizers[t->ptzr] ] = t;
}

void
kax_reader_c::create_packetizers() {
  m_in->save_pos();

  for (auto &track : m_tracks)
    create_packetizer(track->tnum);

  if (!g_segment_title_set && !m_title.empty()) {
    g_segment_title     = m_title;
    g_segment_title_set = true;
  }

  m_in->restore_pos();
}

void
kax_reader_c::create_avc_es_video_packetizer(kax_track_t *t,
                                             track_info_c &nti) {
  auto ptzr = new avc_es_video_packetizer_c(this, nti);
  set_track_packetizer(t, ptzr);

  ptzr->set_video_pixel_dimensions(t->v_width, t->v_height);

  show_packetizer_info(t->tnum, t->ptzr_ptr);
}

void
kax_reader_c::create_avc_video_packetizer(kax_track_t *t,
                                          track_info_c &nti) {
  if (!nti.m_private_data || !nti.m_private_data->get_size()) {
    create_avc_es_video_packetizer(t, nti);
    return;
  }

  set_track_packetizer(t, new avc_video_packetizer_c(this, nti, t->v_frate, t->v_width, t->v_height));
  show_packetizer_info(t->tnum, t->ptzr_ptr);
}

void
kax_reader_c::create_vc1_video_packetizer(kax_track_t *t,
                                          track_info_c &nti) {
  read_first_frames(t, 1);
  if (   !t->first_frames_data.empty()
      && (4 <= t->first_frames_data[0]->get_size())
      && !mtx::vc1::is_marker(get_uint32_be(t->first_frames_data[0]->get_buffer()))) {
    init_passthrough_packetizer(t, nti);
    return;
  }

  set_track_packetizer(t, new vc1_video_packetizer_c(this, nti));
  show_packetizer_info(t->tnum, t->ptzr_ptr);

  if (t->private_data && (sizeof(alBITMAPINFOHEADER) < t->private_data->get_size()))
    t->ptzr_ptr->process(new packet_t(new memory_c(t->private_data->get_buffer() + sizeof(alBITMAPINFOHEADER), t->private_data->get_size() - sizeof(alBITMAPINFOHEADER), false)));
}

void
kax_reader_c::read_first_frames(kax_track_t *t,
                                unsigned num_wanted) {
  if (t->first_frames_data.size() >= num_wanted)
    return;

  std::map<int64_t, unsigned int> frames_by_track_id;

  try {
    while (true) {
      auto cluster = std::unique_ptr<KaxCluster>{m_in_file->read_next_cluster()};
      if (!cluster)
        return;

      KaxClusterTimecode *ctc = static_cast<KaxClusterTimecode *> (cluster->FindFirstElt(EBML_INFO(KaxClusterTimecode), false));
      if (ctc)
        cluster->InitTimecode(ctc->GetValue(), m_tc_scale);

      size_t bgidx;
      for (bgidx = 0; bgidx < cluster->ListSize(); bgidx++) {
        if (Is<KaxSimpleBlock>((*cluster)[bgidx])) {
          KaxSimpleBlock *block_simple = static_cast<KaxSimpleBlock *>((*cluster)[bgidx]);

          block_simple->SetParent(*cluster);
          kax_track_t *block_track = find_track_by_num(block_simple->TrackNum());

          if (!block_track || (0 == block_simple->NumberFrames()))
            continue;

          for (int frame_idx = 0, num_frames = block_simple->NumberFrames(); frame_idx < num_frames; ++frame_idx) {
            frames_by_track_id[ block_simple->TrackNum() ]++;

            if (frames_by_track_id[ block_simple->TrackNum() ] <= block_track->first_frames_data.size())
              continue;

            DataBuffer &data_buffer = block_simple->GetBuffer(frame_idx);
            block_track->first_frames_data.push_back(memory_cptr(new memory_c(data_buffer.Buffer(), data_buffer.Size())));
            block_track->content_decoder.reverse(block_track->first_frames_data.back(), CONTENT_ENCODING_SCOPE_BLOCK);
            block_track->first_frames_data.back()->grab();
          }

        } else if (Is<KaxBlockGroup>((*cluster)[bgidx])) {
          KaxBlockGroup *block_group = static_cast<KaxBlockGroup *>((*cluster)[bgidx]);
          KaxBlock *block            = static_cast<KaxBlock *>(block_group->FindFirstElt(EBML_INFO(KaxBlock), false));

          if (!block)
            continue;

          block->SetParent(*cluster);
          kax_track_t *block_track = find_track_by_num(block->TrackNum());

          if (!block_track || (0 == block->NumberFrames()))
            continue;

          for (int frame_idx = 0, num_frames = block->NumberFrames(); frame_idx < num_frames; ++frame_idx) {
            frames_by_track_id[ block->TrackNum() ]++;

            if (frames_by_track_id[ block->TrackNum() ] <= block_track->first_frames_data.size())
              continue;

            DataBuffer &data_buffer = block->GetBuffer(frame_idx);
            block_track->first_frames_data.push_back(memory_cptr(new memory_c(data_buffer.Buffer(), data_buffer.Size())));
            block_track->content_decoder.reverse(block_track->first_frames_data.back(), CONTENT_ENCODING_SCOPE_BLOCK);
            block_track->first_frames_data.back()->grab();
          }
        }
      }

      if (t->first_frames_data.size() >= num_wanted)
        break;
    }
  } catch (...) {
  }
}

file_status_e
kax_reader_c::read(generic_packetizer_c *requested_ptzr,
                   bool force) {
  if (m_tracks.empty() || (FILE_STATUS_DONE == m_file_status))
    return FILE_STATUS_DONE;

  auto num_queued_bytes = get_queued_bytes();

  if (20 * 1024 * 1024 < num_queued_bytes) {
    auto requested_ptzr_track = m_ptzr_to_track_map[requested_ptzr];
    if (   !requested_ptzr_track
        || (!force && ('a' != requested_ptzr_track->type) && ('v' != requested_ptzr_track->type))
        || ( force && (128 * 1024 * 1024 >= num_queued_bytes)))
      return FILE_STATUS_HOLDING;
  }

  try {
    KaxCluster *cluster = m_in_file->read_next_cluster();
    if (!cluster) {
      flush_packetizers();

      m_file_status = FILE_STATUS_DONE;
      return FILE_STATUS_DONE;
    }

    auto cluster_ts = FindChildValue<KaxClusterTimecode>(cluster);
    cluster->InitTimecode(cluster_ts, m_tc_scale);

    if (-1 == m_first_timestamp) {
      m_first_timestamp = cluster_ts * m_tc_scale;

      // If we're appending this file to another one then the core
      // needs the timestamps shifted to zero.
      if (m_appending && m_chapters && (0 < m_first_timestamp))
        mtx::chapters::adjust_timestamps(*m_chapters, -m_first_timestamp);
    }

    size_t bgidx;
    for (bgidx = 0; bgidx < cluster->ListSize(); bgidx++) {
      EbmlElement *element = (*cluster)[bgidx];

      if (Is<KaxSimpleBlock>(element))
        process_simple_block(cluster, static_cast<KaxSimpleBlock *>(element));

      else if (Is<KaxBlockGroup>(element))
        process_block_group(cluster, static_cast<KaxBlockGroup *>(element));
    }

    delete cluster;

  } catch (...) {
    mxwarn(boost::format("%1% %2% %3%\n")
           % (boost::format(Y("%1%: an unknown exception occurred.")) % "kax_reader_c::read()")
           % Y("This usually indicates a damaged file structure.") % Y("The file will not be processed further."));
    flush_packetizers();
    return FILE_STATUS_DONE;
  }

  return FILE_STATUS_MOREDATA;
}

void
kax_reader_c::process_simple_block(KaxCluster *cluster,
                                   KaxSimpleBlock *block_simple) {
  int64_t block_duration = -1;
  int64_t block_bref     = VFT_IFRAME;
  int64_t block_fref     = VFT_NOBFRAME;

  block_simple->SetParent(*cluster);
  auto block_track     = find_track_by_num(block_simple->TrackNum());
  auto block_timestamp = mtx::math::to_signed(block_simple->GlobalTimecode()) + m_global_timestamp_offset;

  if (!block_track) {
    mxwarn_fn(m_ti.m_fname,
              boost::format(Y("A block was found at timestamp %1% for track number %2%. However, no headers where found for that track number. "
                              "The block will be skipped.\n")) % format_timestamp(block_timestamp) % block_simple->TrackNum());
    return;
  }

  if (0 != block_track->v_frate)
    block_duration = 1000000000.0 / block_track->v_frate;
  int64_t frame_duration = (block_duration == -1) ? 0 : block_duration;

  if (('s' == block_track->type) && (-1 == block_duration))
    block_duration = 0;

  if (block_track->ignore_duration_hack) {
    frame_duration = 0;
    if (0 < block_duration)
      block_duration = 0;
  }

  auto key_flag         = block_simple->IsKeyframe();
  auto discardable_flag = block_simple->IsDiscardable();

  if (!key_flag) {
    if (discardable_flag)
      block_fref = block_track->previous_timestamp;
    else
      block_bref = block_track->previous_timestamp;
  }

  m_last_timestamp = block_timestamp;
  if (0 < block_simple->NumberFrames())
    m_in_file->set_last_timestamp(m_last_timestamp + (block_simple->NumberFrames() - 1) * frame_duration);

  // If we're appending this file to another one then the core
  // needs the timestamps shifted to zero.
  if (m_appending)
    m_last_timestamp -= m_first_timestamp;

  if ((-1 != block_track->ptzr) && block_track->passthrough) {
    // The handling for passthrough is a bit different. We don't have
    // any special cases, e.g. 0 terminating a string for the subs
    // and stuff. Just pass everything through as it is.
    size_t i;
    for (i = 0; block_simple->NumberFrames() > i; ++i) {
      DataBuffer &data_buffer = block_simple->GetBuffer(i);
      memory_cptr data(new memory_c(data_buffer.Buffer(), data_buffer.Size(), false));
      block_track->content_decoder.reverse(data, CONTENT_ENCODING_SCOPE_BLOCK);

      packet_cptr packet(new packet_t(data, m_last_timestamp + i * frame_duration, block_duration, block_bref, block_fref));
      packet->key_flag         = key_flag;
      packet->discardable_flag = discardable_flag;

      static_cast<passthrough_packetizer_c *>(PTZR(block_track->ptzr))->process(packet);
    }

  } else if (-1 != block_track->ptzr) {
    size_t i;
    for (i = 0; i < block_simple->NumberFrames(); i++) {
      DataBuffer &data_buffer = block_simple->GetBuffer(i);
      memory_cptr data(new memory_c(data_buffer.Buffer(), data_buffer.Size(), false));
      block_track->content_decoder.reverse(data, CONTENT_ENCODING_SCOPE_BLOCK);

      if (('s' == block_track->type) && ('t' == block_track->sub_type)) {
        if ((2 < data->get_size()) || ((0 < data->get_size()) && (' ' != *data->get_buffer()) && (0 != *data->get_buffer()) && !iscr(*data->get_buffer()))) {
          auto packet              = std::make_shared<packet_t>(data, m_last_timestamp, block_duration, block_bref, block_fref);
          packet->key_flag         = key_flag;
          packet->discardable_flag = discardable_flag;

          PTZR(block_track->ptzr)->process(packet);
        }

      } else {
        packet_cptr packet(new packet_t(data, m_last_timestamp + i * frame_duration, block_duration, block_bref, block_fref));
        packet->key_flag         = key_flag;
        packet->discardable_flag = discardable_flag;
        PTZR(block_track->ptzr)->process(packet);
      }
    }
  }

  block_track->previous_timestamp  = m_last_timestamp;
  block_track->units_processed    += block_simple->NumberFrames();
}

void
kax_reader_c::process_block_group_common(KaxBlockGroup *block_group,
                                         packet_t *packet,
                                         kax_track_t &block_track) {
  auto codec_state     = FindChild<KaxCodecState>(block_group);
  auto discard_padding = FindChild<KaxDiscardPadding>(block_group);
  auto blockadd        = FindChild<KaxBlockAdditions>(block_group);

  if (codec_state)
    packet->codec_state = memory_c::clone(codec_state->GetBuffer(), codec_state->GetSize());

  if (discard_padding)
    packet->discard_padding = timestamp_c::ns(discard_padding->GetValue());

  if (!blockadd)
    return;

  for (auto &child : *blockadd) {
    if (!(Is<KaxBlockMore>(child)))
      continue;

    auto blockmore     = static_cast<KaxBlockMore *>(child);
    auto blockadd_data = &GetChild<KaxBlockAdditional>(*blockmore);
    auto blockadded    = std::make_shared<memory_c>(blockadd_data->GetBuffer(), blockadd_data->GetSize(), false);
    block_track.content_decoder.reverse(blockadded, CONTENT_ENCODING_SCOPE_BLOCK);

    packet->data_adds.push_back(blockadded);
  }
}

void
kax_reader_c::process_block_group(KaxCluster *cluster,
                                  KaxBlockGroup *block_group) {
  auto block = FindChild<KaxBlock>(block_group);
  if (!block)
    return;

  block->SetParent(*cluster);
  auto block_track     = find_track_by_num(block->TrackNum());
  auto block_timestamp = mtx::math::to_signed(block->GlobalTimecode()) + m_global_timestamp_offset;

  if (!block_track) {
    mxwarn_fn(m_ti.m_fname,
              boost::format(Y("A block was found at timestamp %1% for track number %2%. However, no headers where found for that track number. "
                              "The block will be skipped.\n")) % format_timestamp(block_timestamp) % block->TrackNum());
    return;
  }

  auto duration       = FindChild<KaxBlockDuration>(block_group);
  auto block_duration = duration             ? static_cast<int64_t>(duration->GetValue() * m_tc_scale / block->NumberFrames())
                      : block_track->v_frate ? static_cast<int64_t>(1000000000.0 / block_track->v_frate)
                      :                        int64_t{-1};
  auto frame_duration = -1 == block_duration ? int64_t{0} : block_duration;
  m_last_timestamp     = block_timestamp;

  if (0 < block->NumberFrames())
    m_in_file->set_last_timestamp(m_last_timestamp + (block->NumberFrames() - 1) * frame_duration);

  // If we're appending this file to another one then the core
  // needs the timestamps shifted to zero.
  if (m_appending)
    m_last_timestamp -= m_first_timestamp;

  if (-1 == block_track->ptzr)
    return;

  auto block_bref = int64_t{VFT_IFRAME};
  auto block_fref = int64_t{VFT_NOBFRAME};
  bool bref_found = false;
  bool fref_found = false;
  auto ref_block  = FindChild<KaxReferenceBlock>(block_group);

  while (ref_block) {
    if (0 >= ref_block->GetValue()) {
      block_bref = ref_block->GetValue() * m_tc_scale;
      bref_found = true;
    } else {
      block_fref = ref_block->GetValue() * m_tc_scale;
      fref_found = true;
    }

    ref_block = FindNextChild(*block_group, *ref_block);
  }

  if (('s' == block_track->type) && (-1 == block_duration))
    block_duration = 0;

  if (block_track->ignore_duration_hack) {
    frame_duration = 0;
    if (0 < block_duration)
      block_duration = 0;
  }

  if (block_track->passthrough) {
    // The handling for passthrough is a bit different. We don't have
    // any special cases, e.g. 0 terminating a string for the subs
    // and stuff. Just pass everything through as it is.
    if (bref_found)
      block_bref += m_last_timestamp;
    if (fref_found)
      block_fref += m_last_timestamp;

    size_t i;
    for (i = 0; i < block->NumberFrames(); i++) {
      auto &data_buffer = block->GetBuffer(i);
      auto data         = std::make_shared<memory_c>(data_buffer.Buffer(), data_buffer.Size(), false);
      block_track->content_decoder.reverse(data, CONTENT_ENCODING_SCOPE_BLOCK);

      auto packet                = std::make_shared<packet_t>(data, m_last_timestamp + i * frame_duration, block_duration, block_bref, block_fref);
      packet->duration_mandatory = duration;

      process_block_group_common(block_group, packet.get(), *block_track);

      static_cast<passthrough_packetizer_c *>(PTZR(block_track->ptzr))->process(packet);
    }

    return;
  }

  if (bref_found)
    block_bref += m_last_timestamp;
  if (fref_found)
    block_fref += m_last_timestamp;

  for (auto block_idx = 0u, num_frames = block->NumberFrames(); block_idx < num_frames; ++block_idx) {
    auto &data_buffer = block->GetBuffer(block_idx);
    auto data         = std::make_shared<memory_c>(data_buffer.Buffer(), data_buffer.Size(), false);
    block_track->content_decoder.reverse(data, CONTENT_ENCODING_SCOPE_BLOCK);

    if (('s' == block_track->type) && ('t' == block_track->sub_type)) {
      if ((2 < data->get_size()) || ((0 < data->get_size()) && (' ' != *data->get_buffer()) && (0 != *data->get_buffer()) && !iscr(*data->get_buffer()))) {
        auto packet = std::make_shared<packet_t>(data, m_last_timestamp, block_duration, block_bref, block_fref);

        process_block_group_common(block_group, packet.get(), *block_track);

        PTZR(block_track->ptzr)->process(packet);
      }

    } else {
      auto packet = std::make_shared<packet_t>(data, m_last_timestamp + block_idx * frame_duration, block_duration, block_bref, block_fref);

      if ((duration) && !duration->GetValue())
        packet->duration_mandatory = true;

      process_block_group_common(block_group, packet.get(), *block_track);

      PTZR(block_track->ptzr)->process(packet);
    }
  }

  block_track->previous_timestamp  = m_last_timestamp;
  block_track->units_processed    += block->NumberFrames();
}

int
kax_reader_c::get_progress() {
  if (0 != m_segment_duration)
    return std::min(m_last_timestamp, m_segment_duration) * 100 / m_segment_duration;

  return 100 * m_in->getFilePointer() / m_size;
}

void
kax_reader_c::set_headers() {
  generic_reader_c::set_headers();

  for (auto &track : m_tracks)
    if ((-1 != track->ptzr) && track->passthrough)
      PTZR(track->ptzr)->get_track_entry()->EnableLacing(track->lacing_flag);
}

void
kax_reader_c::determine_minimum_timestamps() {
  if (m_tracks.empty())
    return;

  m_in->save_pos();
  mtx::at_scope_exit_c restore{[this]() { m_in->restore_pos(); }};

  std::unordered_map<uint64_t, kax_track_cptr> tracks_by_number;

  timestamp_c first_timestamp, last_timestamp;
  auto probe_time_limit = timestamp_c::s(10);
  auto video_time_limit = timestamp_c::s(1);

  for (auto &track : m_tracks)
    tracks_by_number[track->track_number] = track;

  while (true) {
    try {
      auto cluster = std::shared_ptr<KaxCluster>(m_in_file->read_next_cluster());
      if (!cluster)
        return;

      auto cluster_ts = FindChildValue<KaxClusterTimecode>(*cluster);
      cluster->InitTimecode(cluster_ts, m_tc_scale);

      for (auto const &element : *cluster) {
        uint64_t track_number{};

        if (Is<KaxSimpleBlock>(element)) {
          auto block = static_cast<KaxSimpleBlock *>(element);
          block->SetParent(*cluster);

          last_timestamp = timestamp_c::ns(mtx::math::to_signed(block->GlobalTimecode()));
          track_number   = block->TrackNum();

        } else if (Is<KaxBlockGroup>(element)) {
          auto block = FindChild<KaxBlock>(static_cast<KaxBlockGroup *>(element));
          if (!block)
            continue;

          block->SetParent(*cluster);

          last_timestamp = timestamp_c::ns(mtx::math::to_signed(block->GlobalTimecode()));
          track_number   = block->TrackNum();

        } else
          continue;

        if (!first_timestamp.valid())
          first_timestamp = last_timestamp;

        if ((last_timestamp - first_timestamp) >= probe_time_limit)
          return;

        auto &track = tracks_by_number[track_number];
        if (!track)
          continue;

        auto &recorded_timestamp = m_minimum_timestamps_by_track_number[track_number];
        if (!recorded_timestamp.valid() || (last_timestamp < recorded_timestamp))
          recorded_timestamp = last_timestamp;

        if (   (track->type == 'v')
            && ((last_timestamp - recorded_timestamp) < video_time_limit))
          continue;

        tracks_by_number.erase(track_number);
        if (tracks_by_number.empty())
          return;
      }

    } catch (...) {
      break;
    }
  }
}

void
kax_reader_c::determine_global_timestamp_offset_to_apply() {
  timestamp_c global_minimum_timestamp;

  for (auto const &pair : m_minimum_timestamps_by_track_number) {
    if (!pair.second.valid())
      continue;

    if (!global_minimum_timestamp.valid() || (pair.second < global_minimum_timestamp))
      global_minimum_timestamp = pair.second;
  }

  if (global_minimum_timestamp.valid() && (global_minimum_timestamp < timestamp_c::ns(0)))
    m_global_timestamp_offset = global_minimum_timestamp.abs().to_ns();
}

void
kax_reader_c::identify() {
  auto info = mtx::id::info_c{};

  if (!m_title.empty())
    info.add(mtx::id::title, m_title);
  if (0 != m_segment_duration)
    info.add(mtx::id::duration, m_segment_duration);

  auto add_uid_info = [&info](memory_cptr const &uid, std::string const &property) {
    if (uid)
      info.add(property, to_hex(uid, true));
  };
  add_uid_info(m_segment_uid,          mtx::id::segment_uid);
  add_uid_info(m_next_segment_uid,     mtx::id::next_segment_uid);
  add_uid_info(m_previous_segment_uid, mtx::id::previous_segment_uid);

  info.set(mtx::id::muxing_application,  m_muxing_app);
  info.set(mtx::id::writing_application, m_raw_writing_app);
  if (m_muxing_date_epoch) {
    info.add(mtx::id::date_utc,   mtx::date_time::format_epoch_time_iso_8601(m_muxing_date_epoch.get(), mtx::date_time::epoch_timezone_e::UTC));
    info.add(mtx::id::date_local, mtx::date_time::format_epoch_time_iso_8601(m_muxing_date_epoch.get(), mtx::date_time::epoch_timezone_e::local));
  }

  id_result_container(info.get());

  for (auto &track : m_tracks) {
    if (!track->ok)
      continue;

    info = mtx::id::info_c{};

    info.add(mtx::id::number,               track->track_number);
    info.add(mtx::id::uid,                  track->track_uid);
    info.add(mtx::id::codec_id,             track->codec_id);
    info.set(mtx::id::codec_private_length, track->private_data ? track->private_data->get_size() : 0u);
    info.add(mtx::id::codec_delay,          track->codec_delay.to_ns(0));
    info.add(mtx::id::language,             track->language);
    info.add(mtx::id::track_name,           track->track_name);
    info.add(mtx::id::stereo_mode,          static_cast<int>(track->v_stereo_mode), static_cast<int>(stereo_mode_c::unspecified));
    info.add(mtx::id::default_duration,     track->default_duration);
    info.set(mtx::id::default_track,        track->default_track ? true : false);
    info.set(mtx::id::forced_track,         track->forced_track  ? true : false);
    info.set(mtx::id::enabled_track,        track->enabled_track ? true : false);

    if (track->private_data && (0 != track->private_data->get_size()))
      info.add(mtx::id::codec_private_data, to_hex(track->private_data->get_buffer(), track->private_data->get_size(), true));

    if ((0 != track->v_width) && (0 != track->v_height))
      info.add(mtx::id::pixel_dimensions, boost::format("%1%x%2%") % track->v_width % track->v_height);

    if ((0 != track->v_dwidth) && (0 != track->v_dheight))
      info.add(mtx::id::display_dimensions, boost::format("%1%x%2%") % track->v_dwidth % track->v_dheight);

    if ((0 != track->v_pcleft) || (0 != track->v_pctop) || (0 != track->v_pcright) || (0 != track->v_pcbottom))
      info.add(mtx::id::cropping, boost::format("%1%,%2%,%3%,%4%") % track->v_pcleft % track->v_pctop % track->v_pcright % track->v_pcbottom);

    if (track->codec.is(codec_c::type_e::V_MPEG4_P10))
      info.add(mtx::id::packetizer, track->ms_compat ? mtx::id::mpeg4_p10_es_video : mtx::id::mpeg4_p10_video);
    else if (track->codec.is(codec_c::type_e::V_MPEGH_P2))
      info.add(mtx::id::packetizer, track->ms_compat ? mtx::id::mpegh_p2_es_video  : mtx::id::mpegh_p2_video);

    if ('a' == track->type) {
      info.add(mtx::id::audio_sampling_frequency, static_cast<int64_t>(track->a_sfreq));
      info.add(mtx::id::audio_channels,           track->a_channels);
      info.add(mtx::id::audio_bits_per_sample,    track->a_bps);

    } else if ('s' == track->type) {
      if (track->codec.is(codec_c::type_e::S_SRT) || track->codec.is(codec_c::type_e::S_SSA_ASS) || track->codec.is(codec_c::type_e::S_KATE)) {
        info.add(mtx::id::text_subtitles, true);
        info.add(mtx::id::encoding, "UTF-8");
      }
    }

    if (track->content_decoder.has_encodings())
      info.add(mtx::id::content_encoding_algorithms, track->content_decoder.descriptive_algorithm_list());

    std::string codec_info;
    if (track->codec)
      codec_info = track->codec.get_name();

    else if (track->ms_compat) {
      if (track->type == 'v') {
        // auto fourcc_str = fourcc_c{track->v_fourcc}.description();
        // codec_info            = track->codec.get_name(fourcc_str);

        codec_info = fourcc_c{track->v_fourcc}.description();

      } else
        codec_info = (boost::format(Y("unknown, format tag 0x%|1$04x|")) % track->a_formattag).str();

    } else
      codec_info = track->codec_id;

    track->add_track_tags_to_identification(info);

    auto &minimum_timestamp = m_minimum_timestamps_by_track_number[track->track_number];
    if (minimum_timestamp.valid())
      info.set(mtx::id::minimum_timestamp, minimum_timestamp.to_ns());

    id_result_track(track->tnum,
                      track->type == 'v' ? ID_RESULT_TRACK_VIDEO
                    : track->type == 'a' ? ID_RESULT_TRACK_AUDIO
                    : track->type == 'b' ? ID_RESULT_TRACK_BUTTONS
                    : track->type == 's' ? ID_RESULT_TRACK_SUBTITLES
                    :                      Y("unknown"),
                    codec_info, info.get());
  }

  for (auto &attachment : g_attachments)
    id_result_attachment(attachment->ui_id, attachment->mime_type, attachment->data->get_size(), attachment->name, attachment->description, attachment->id);

  if (m_chapters)
    id_result_chapters(mtx::chapters::count_atoms(*m_chapters));

  if (m_tags)
    id_result_tags(ID_RESULT_GLOBAL_TAGS_ID, mtx::tags::count_simple(*m_tags));

  for (auto &track : m_tracks)
    if (track->ok && track->tags)
      id_result_tags(track->tnum, mtx::tags::count_simple(*track->tags));
}

void
kax_reader_c::add_available_track_ids() {
  for (auto &track : m_tracks)
    add_available_track_id(track->tnum);
}

void
kax_reader_c::set_track_packetizer(kax_track_t *t,
                                   generic_packetizer_c *ptzr) {
  t->ptzr     = add_packetizer(ptzr);
  t->ptzr_ptr = PTZR(t->ptzr);
}
