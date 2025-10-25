/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Matroska reader

   Written by Moritz Bunkus <mo@bunkus.online>.
   Modified by Steve Lhomme <steve.lhomme@free.fr>.
*/

#include "common/common_pch.h"

#include <cmath>

#include <QDateTime>
#include <QRegularExpression>
#include <QTimeZone>

#include <ebml/EbmlCrc32.h>
#include <ebml/EbmlContexts.h>
#include <ebml/EbmlDummy.h>
#include <ebml/EbmlHead.h>
#include <ebml/EbmlStream.h>
#if LIBEBML_VERSION < 0x020000
# include <ebml/EbmlSubHead.h>
#endif
#include <ebml/EbmlVoid.h>

#include <matroska/KaxBlock.h>
#include <matroska/KaxBlockData.h>
#include <matroska/KaxCluster.h>
#include <matroska/KaxContexts.h>
#include <matroska/KaxCuesData.h>
#if LIBMATROSKA_VERSION < 0x020000
# include <matroska/KaxInfoData.h>
#endif
#include <matroska/KaxSeekHead.h>
#include <matroska/KaxSegment.h>
#include <matroska/KaxTracks.h>

#include "avilib.h"
#include "common/alac.h"
#include "common/at_scope_exit.h"
#include "common/audio_emphasis.h"
#include "common/chapters/chapters.h"
#include "common/codec.h"
#include "common/container.h"
#include "common/date_time.h"
#include "common/debugging.h"
#include "common/ebml.h"
#include "common/endian.h"
#include "common/hacks.h"
#include "common/kax_analyzer.h"
#include "common/math.h"
#include "common/mime.h"
#include "common/mm_io.h"
#include "common/qt.h"
#include "common/random.h"
#include "common/strings/formatting.h"
#include "common/strings/parsing.h"
#include "common/strings/utf8.h"
#include "common/tags/tags.h"
#include "common/tags/vorbis.h"
#include "common/id_info.h"
#include "common/vobsub.h"
#include "input/r_matroska.h"
#include "merge/file_status.h"
#include "merge/input_x.h"
#include "merge/output_control.h"
#include "output/p_aac.h"
#include "output/p_ac3.h"
#include "output/p_alac.h"
#include "output/p_av1.h"
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
#include "output/p_prores.h"
#include "output/p_ssa.h"
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

namespace {
unsigned int
writing_app_ver(unsigned int major,
                unsigned int minor,
                unsigned int patchlevel,
                unsigned int errata) {
  return (major << 24) | (minor << 16) | (patchlevel << 8) | errata;
}

auto
map_track_type(char c) {
  return c == 'a' ? track_audio
       : c == 'b' ? track_buttons
       : c == 'v' ? track_video
       :            track_subtitle;
}

auto
map_track_type_string(char c) {
  return c == '?' ? Y("unknown")
       : c == 'a' ? Y("audio")
       : c == 'b' ? Y("buttons")
       : c == 'v' ? Y("video")
       :            Y("subtitle");
}

template<typename T>
void
remap_uids(libebml::EbmlMaster &master,
           std::unordered_map<uint64_t, uint64_t> const &map) {
  for (auto const &child : master) {
    if (is_type<T>(child)) {
      auto &e_uint = static_cast<libebml::EbmlUInteger &>(*child);
      auto itr     = map.find(e_uint.GetValue());

      if (itr != map.end())
        e_uint.SetValue(itr->second);

    } else if (dynamic_cast<libebml::EbmlMaster *>(child)) {
      remap_uids<T>(static_cast<libebml::EbmlMaster &>(*child), map);
    }
  }
}

constexpr auto MAGIC_MKV = 0x1a45dfa3;

} // anonymous namespace

void
kax_track_t::handle_packetizer_display_dimensions() {
  // If user hasn't set an aspect ratio via the command line and the
  // source file provides display width/height paramaters then use
  // these and signal the packetizer not to extract the dimensions
  // from the bitstream.
  if ((0 != v_dwidth) && (0 != v_dheight))
    ptzr_ptr->set_video_display_dimensions(v_dwidth, v_dheight, v_dunit.value_or(generic_packetizer_c::ddu_pixels), option_source_e::container);
}

void
kax_track_t::handle_packetizer_pixel_cropping() {
  if ((0 < v_pcleft) || (0 < v_pctop) || (0 < v_pcright) || (0 < v_pcbottom))
    ptzr_ptr->set_video_pixel_cropping(v_pcleft, v_pctop, v_pcright, v_pcbottom, option_source_e::container);
}

void
kax_track_t::handle_packetizer_color() {
  if (v_color_matrix)
    ptzr_ptr->set_video_color_matrix(*v_color_matrix, option_source_e::container);
  if (v_bits_per_channel)
    ptzr_ptr->set_video_bits_per_channel(*v_bits_per_channel, option_source_e::container);
  if (v_chroma_subsample.hori || v_chroma_subsample.vert)
    ptzr_ptr->set_video_chroma_subsample(v_chroma_subsample, option_source_e::container);
  if (v_cb_subsample.hori || v_cb_subsample.vert)
    ptzr_ptr->set_video_cb_subsample(v_cb_subsample, option_source_e::container);
  if (v_chroma_siting.hori || v_chroma_siting.vert)
    ptzr_ptr->set_video_chroma_siting(v_chroma_siting, option_source_e::container);
  if (v_color_range)
    ptzr_ptr->set_video_color_range(*v_color_range, option_source_e::container);
  if (v_transfer_character)
    ptzr_ptr->set_video_color_transfer_character(*v_transfer_character, option_source_e::container);
  if (v_color_primaries)
    ptzr_ptr->set_video_color_primaries(*v_color_primaries, option_source_e::container);
  if (v_max_cll)
    ptzr_ptr->set_video_max_cll(*v_max_cll, option_source_e::container);
  if (v_max_fall)
    ptzr_ptr->set_video_max_fall(*v_max_fall, option_source_e::container);
  if (   v_chroma_coordinates.red_x   || v_chroma_coordinates.red_y
      || v_chroma_coordinates.green_x || v_chroma_coordinates.green_y
      || v_chroma_coordinates.blue_x  || v_chroma_coordinates.blue_y) {
    ptzr_ptr->set_video_chroma_coordinates(v_chroma_coordinates, option_source_e::container);
  }
  if (v_white_color_coordinates.hori && v_white_color_coordinates.vert)
    ptzr_ptr->set_video_white_color_coordinates(v_white_color_coordinates, option_source_e::container);
  if (v_max_luminance)
    ptzr_ptr->set_video_max_luminance(*v_max_luminance, option_source_e::container);
  if (v_min_luminance)
    ptzr_ptr->set_video_min_luminance(*v_min_luminance, option_source_e::container);

  if (v_projection_type)
    ptzr_ptr->set_video_projection_type(*v_projection_type, option_source_e::container);
  if (v_projection_private)
    ptzr_ptr->set_video_projection_private(v_projection_private, option_source_e::container);
  if (v_projection_pose_yaw)
    ptzr_ptr->set_video_projection_pose_yaw(*v_projection_pose_yaw, option_source_e::container);
  if (v_projection_pose_pitch)
    ptzr_ptr->set_video_projection_pose_pitch(*v_projection_pose_pitch, option_source_e::container);
  if (v_projection_pose_roll)
    ptzr_ptr->set_video_projection_pose_roll(*v_projection_pose_roll, option_source_e::container);

  if (codec_id == MKV_V_UNCOMPRESSED)
    ptzr_ptr->set_video_color_space(v_color_space, option_source_e::container);
}

void
kax_track_t::handle_packetizer_field_order() {
  if (v_field_order)
    ptzr_ptr->set_video_field_order(*v_field_order, option_source_e::container);
}

void
kax_track_t::handle_packetizer_stereo_mode() {
  if (stereo_mode_c::unspecified != v_stereo_mode)
    ptzr_ptr->set_video_stereo_mode(v_stereo_mode, option_source_e::container);
}

void
kax_track_t::handle_packetizer_alpha_mode() {
  if (v_alpha_mode)
    ptzr_ptr->set_video_alpha_mode(*v_alpha_mode, option_source_e::container);
}

void
kax_track_t::handle_packetizer_pixel_dimensions() {
  if ((0 == v_width) || (0 == v_height))
    return;

  ptzr_ptr->set_video_pixel_dimensions(v_width, v_height);
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

void
kax_track_t::handle_packetizer_block_addition_mapping() {
  ptzr_ptr->set_block_addition_mappings(block_addition_mappings);
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

  if (std::gcd(v_dwidth, v_dheight) != 1)
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
kax_track_t::get_source_id_from_track_statistics_tags() {
  if (!tags)
    return;

  for (auto const &tags_child : *tags) {
    if (!dynamic_cast<libmatroska::KaxTag *>(tags_child))
      continue;

    auto value = mtx::tags::get_simple_value("SOURCE_ID", static_cast<libmatroska::KaxTag &>(*tags_child));
    if (value.empty())
      continue;

    source_id = value;
    return;
  }
}

void
kax_track_t::discard_track_statistics_tags() {
  if (!tags)
    return;

  mtx::tags::remove_track_statistics(tags.get(), track_uid);

  if (!tags->ListSize())
    tags.reset();
}

void
kax_track_t::register_use_of_webm_block_addition_id(uint64_t id) {
  // So far we only know about IDs 1 & 4. For 1 no mapping must be added.

  if ((id != 4) || m_registered_used_of_webm_block_addition_id[id])
    return;

  m_registered_used_of_webm_block_addition_id[id] = true;

  for (auto const &mapping : block_addition_mappings)
    if (mapping.id_value.value_or(1) == id)
      return;

  block_addition_mapping_t mapping;
  mapping.id_type  = 4;
  mapping.id_value = id;

  block_addition_mappings.push_back(mapping);
  ptzr_ptr->set_block_addition_mappings(block_addition_mappings);

  rerender_track_headers();
}


/*
   Probes a file by simply comparing the first four bytes to the EBML
   head signature.
*/
bool
kax_reader_c::probe_file() {
  uint8_t data[4];
  return (m_in->read(data, 4) == 4) && (get_uint32_be(data) == MAGIC_MKV);
}

kax_reader_c::kax_reader_c() {
  init_l1_position_storage(m_deferred_l1_positions);
  init_l1_position_storage(m_handled_l1_positions);
}

void
kax_reader_c::init_l1_position_storage(deferred_positions_t &storage) {
  storage[dl1t_attachments] = std::vector<int64_t>();
  storage[dl1t_chapters]    = std::vector<int64_t>();
  storage[dl1t_tags]        = std::vector<int64_t>();
  storage[dl1t_tracks]      = std::vector<int64_t>();
  storage[dl1t_seek_head]   = std::vector<int64_t>();
  storage[dl1t_cues]        = std::vector<int64_t>();
}

bool
kax_reader_c::packets_available() {
  for (auto &track : m_tracks)
    if ((-1 != track->ptzr) && !ptzr(track->ptzr).packet_available())
      return false;

  return !m_tracks.empty();
}

kax_track_t *
kax_reader_c::find_track_by_num(uint64_t n,
                                kax_track_t *c) {
  auto itr = std::find_if(m_tracks.begin(), m_tracks.end(), [n, c](auto &track) { return (track->track_number == n) && (track.get() != c); });
  return itr == m_tracks.end() ? nullptr : itr->get();
}

kax_track_t *
kax_reader_c::find_track_by_uid(uint64_t uid,
                                kax_track_t *c) {
  auto itr = std::find_if(m_tracks.begin(), m_tracks.end(), [uid, c](auto &track) { return (track->track_uid == uid) && (track.get() != c); });
  return itr == m_tracks.end() ? nullptr : itr->get();
}

bool
kax_reader_c::unlace_vorbis_private_data(kax_track_t *t,
                                         uint8_t *buffer,
                                         int size) {
  try {
    t->headers = unlace_memory_xiph(memory_c::borrow(buffer, size));
    for (auto const &header : t->headers)
      header->take_ownership();

  } catch (...) {
    return false;
  }

  return true;
}

bool
kax_reader_c::verify_acm_audio_track(kax_track_t *t) {
  if (!t->private_data || (sizeof(alWAVEFORMATEX) > t->private_data->get_size())) {
    if (verbose)
      mxwarn(fmt::format(FY("matroska_reader: The CodecID for track {0} is '{1}', but there was no WAVEFORMATEX struct present. "
                            "Therefore we don't have a format ID to identify the audio codec used.\n"), t->tnum, MKV_A_ACM));
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
      mxwarn(fmt::format(FY("matroska_reader: (MS compatibility mode for track {0}) Matroska says that there are {1} samples per second, "
                            "but WAVEFORMATEX says that there are {2}.\n"), t->tnum, static_cast<int>(t->a_sfreq), u));
    if (0.0 == t->a_sfreq)
      t->a_sfreq = static_cast<double>(u);
  }

  u = get_uint16_le(&wfe->n_channels);
  if (t->a_channels != u) {
    if (verbose)
      mxwarn(fmt::format(FY("matroska_reader: (MS compatibility mode for track {0}) Matroska says that there are {1} channels, "
                            "but the WAVEFORMATEX says that there are {2}.\n"), t->tnum, t->a_channels, u));
    if (0 == t->a_channels)
      t->a_channels = u;
  }

  u = get_uint16_le(&wfe->w_bits_per_sample);
  if (t->a_bps != u) {
    if (verbose && t->codec.is(codec_c::type_e::A_PCM))
      mxwarn(fmt::format(FY("matroska_reader: (MS compatibility mode for track {0}) Matroska says that there are {1} bits per sample, "
                            "but the WAVEFORMATEX says that there are {2}.\n"), t->tnum, t->a_bps, u));
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
    mxwarn(fmt::format(FY("matroska_reader: The CodecID for track {0} is '{1}', but the private codec data does not contain valid headers.\n"), t->tnum, MKV_A_VORBIS));

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

    t->a_channels = t->dts_header.get_total_num_audio_channels();
    t->a_sfreq    = t->dts_header.get_effective_sampling_frequency();
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
  mxwarn(fmt::format(FY("matroska_reader: mkvmerge was not compiled with FLAC support. Ignoring track {0}.\n"), t->tnum));
  return false;
}
#endif

bool
kax_reader_c::verify_vorbis_audio_track(kax_track_t *t) {
  if (!t->private_data || !unlace_vorbis_private_data(t, t->private_data->get_buffer(), t->private_data->get_size())) {
    if (verbose)
      mxwarn(fmt::format(FY("matroska_reader: The CodecID for track {0} is '{1}', but the private codec data does not contain valid headers.\n"), t->tnum, MKV_A_VORBIS));
    return false;
  }

  // mkvmerge around 0.6.x had a bug writing a default duration
  // for Vorbis m_tracks but not the correct durations for the
  // individual packets. This comes back to haunt us because
  // when regenerating the timestamps from lacing those durations
  // are used and add up to too large a value. The result is the
  // usual "timestamp < m_last_timestamp" message.
  // Workaround: do not use durations for such m_tracks.
  if ((m_writing_app == "mkvmerge") && (-1 != m_writing_app_ver) && (writing_app_ver(7, 0, 0, 0) > m_writing_app_ver))
    t->ignore_duration_hack = true;

  handle_vorbis_comments(*t);

  return true;
}

bool
kax_reader_c::verify_opus_audio_track(kax_track_t *t) {
  if (!t->private_data || !t->private_data->get_size()) {
    if (verbose)
      mxwarn(fmt::format(FY("matroska_reader: The CodecID for track {0} is '{1}', but the private codec data does not contain valid headers.\n"), t->tnum, MKV_A_OPUS));
    return false;
  }

  return true;
}

bool
kax_reader_c::verify_truehd_audio_track(kax_track_t *t) {
  try {
    int num_frames_to_probe = 5;

    while (num_frames_to_probe <= 2000) {
      read_first_frames(t, num_frames_to_probe);

      mtx::truehd::parser_c parser;
      for (auto &frame : t->first_frames_data)
        parser.add_data(frame->get_buffer(), frame->get_size());

      while (parser.frame_available()) {
        auto frame = parser.get_next_frame();

        if (frame->is_ac3() || !frame->is_sync())
          continue;

        t->codec = frame->codec();

        return true;
      }

      num_frames_to_probe *= 20;
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
      mxwarn(fmt::format(FY("matroska_reader: The CodecID for track {0} is '{1}', but there was no BITMAPINFOHEADER struct present. "
                            "Therefore we don't have a FourCC to identify the video codec used.\n"),
                         t->tnum, MKV_V_MSCOMP));
    return false;
  }

  t->ms_compat = 1;
  auto bih     = reinterpret_cast<alBITMAPINFOHEADER *>(t->private_data->get_buffer());
  auto u       = get_uint32_le(&bih->bi_width);

  if (t->v_width != u) {
    if (verbose)
      mxwarn(fmt::format(FY("matroska_reader: (MS compatibility mode, track {0}) Matroska says video width is {1}, but the BITMAPINFOHEADER says {2}.\n"),
                         t->tnum, t->v_width, u));
    if (0 == t->v_width)
      t->v_width = u;
  }

  u = get_uint32_le(&bih->bi_height);
  if (t->v_height != u) {
    if (verbose)
      mxwarn(fmt::format(FY("matroska_reader: (MS compatibility mode, track {0}) Matroska says video height is {1}, but the BITMAPINFOHEADER says {2}.\n"),
                         t->tnum, t->v_height, u));
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
    mxwarn(fmt::format(FY("matroska_reader: The CodecID for track {0} is '{1}', but there was no codec private headers.\n"), t->tnum, MKV_V_THEORA));

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
      mxwarn(fmt::format(FY("matroska_reader: The width for track {0} was not set.\n"), t->tnum));
    return;
  }

  if (0 == t->v_height) {
    if (verbose)
      mxwarn(fmt::format(FY("matroska_reader: The height for track {0} was not set.\n"), t->tnum));
    return;
  }

  // This track seems to be ok.
  t->ok = 1;
}

bool
kax_reader_c::verify_dvb_subtitle_track(kax_track_t *t) {
  if (!t->private_data || (t->private_data->get_size() < 4)) {
    mxwarn(fmt::format(FY("matroska_reader: The CodecID for track {0} is '{1}', but the private codec data does not contain valid headers.\n"), t->tnum, t->codec_id));
    return false;
  }

  return true;
}

bool
kax_reader_c::verify_hdmv_textst_subtitle_track(kax_track_t *t) {
  if (!t->private_data || (t->private_data->get_size() < 4)) {
    mxwarn(fmt::format(FY("matroska_reader: The CodecID for track {0} is '{1}', but the private codec data does not contain valid headers.\n"), t->tnum, t->codec_id));
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
    mxwarn(fmt::format(FY("matroska_reader: The CodecID for track {0} is '{1}', but the private codec data does not contain valid headers.\n"), t->codec_id));
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
    mxwarn(fmt::format(FY("matroska_reader: The CodecID for track {0} is '{1}', but there was no private data found.\n"), t->tnum, t->codec_id));

  return false;
}

bool
kax_reader_c::verify_vobsub_subtitle_track(kax_track_t *t) {
  if (t->private_data)
    return true;

  if (!g_identifying && verbose)
    mxwarn_fn(m_ti.m_fname,
              fmt::format("{0} {1}\n",
                          fmt::format(FY("The VobSub subtitle track {0} does not contain its index in the CodecPrivate element."), t->tnum),
                          Y("A default index and with it default settings for the width, height and color palette will be used instead.")));

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
      mxwarn(fmt::format(FY("matroska_reader: The CodecID '{0}' for track {1} is unknown.\n"), t->codec_id, t->tnum));
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
          mxwarn(fmt::format(FY("matroska_reader: unknown demultiplexer type for track {0}: '{1}'\n"), t->tnum, t->type));
        continue;
    }

    if (t->ok && (1 < verbose))
      mxinfo(fmt::format(FY("matroska_reader: Track {0} seems to be ok.\n"), t->tnum));
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
                                 libebml::EbmlElement *l0,
                                 int64_t pos) {
  if (has_deferred_element_been_processed(dl1t_attachments, pos))
    return;

  io->save_pos(pos);
  mtx::at_scope_exit_c restore([io]() { io->restore_pos(); });

  int upper_lvl_el = 0;
  std::shared_ptr<libebml::EbmlElement> l1(m_es->FindNextElement(EBML_CONTEXT(l0), upper_lvl_el, 0xFFFFFFFFL, true));
  auto atts = dynamic_cast<libmatroska::KaxAttachments *>(l1.get());

  if (!atts)
    return;

  libebml::EbmlElement *element_found{};
  upper_lvl_el = 0;

  atts->Read(*m_es, EBML_CLASS_CONTEXT(libmatroska::KaxAttachments), upper_lvl_el, element_found, true);
  if (!found_in(*atts, element_found))
    delete element_found;

  for (auto l1_att : *atts) {
    auto att = dynamic_cast<libmatroska::KaxAttached *>(l1_att);
    if (!att)
      continue;

    ++m_attachment_id;

    auto fdata = find_child<libmatroska::KaxFileData>(att);
    if (!fdata)
      continue;

    auto matt         = std::make_shared<attachment_t>();
    matt->name        = to_utf8(find_child_value<libmatroska::KaxFileName>(att));
    matt->description = to_utf8(find_child_value<libmatroska::KaxFileDescription>(att));
    matt->mime_type   = ::mtx::mime::get_font_mime_type_to_use(find_child_value<libmatroska::KaxMimeType>(att), g_use_legacy_font_mime_types ? mtx::mime::font_mime_type_type_e::legacy : mtx::mime::font_mime_type_type_e::current);
    matt->id          = find_child_value<libmatroska::KaxFileUID>(att);
    matt->data        = memory_c::clone(static_cast<uint8_t *>(fdata->GetBuffer()), fdata->GetSize());

    auto attach_mode  = attachment_requested(m_attachment_id);

    if (   !matt->data->get_size()
        || matt->mime_type.empty()
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
                              libebml::EbmlElement *l0,
                              int64_t pos) {
  if (has_deferred_element_been_processed(dl1t_chapters, pos))
    return;

  io->save_pos(pos);
  mtx::at_scope_exit_c restore([io]() { io->restore_pos(); });

  int upper_lvl_el = 0;
  std::shared_ptr<libebml::EbmlElement> l1(m_es->FindNextElement(EBML_CONTEXT(l0), upper_lvl_el, 0xFFFFFFFFL, true));
  auto tmp_chapters = dynamic_cast<libmatroska::KaxChapters *>(l1.get());

  if (!tmp_chapters)
    return;

  libebml::EbmlElement *element_found{};
  upper_lvl_el = 0;

  tmp_chapters->Read(*m_es, EBML_CLASS_CONTEXT(libmatroska::KaxChapters), upper_lvl_el, element_found, true);
  if (!found_in(*tmp_chapters, element_found))
    delete element_found;

  if (m_regenerate_chapter_uids)
    mtx::chapters::regenerate_uids(*tmp_chapters, m_tags.get());

  if (m_regenerate_track_uids)
    remap_uids<libmatroska::KaxChapterTrackNumber>(*tmp_chapters, m_track_uid_mapping);

  if (!m_chapters)
    m_chapters = mtx::chapters::kax_cptr{new libmatroska::KaxChapters};

  m_chapters->GetElementList().insert(m_chapters->begin(), tmp_chapters->begin(), tmp_chapters->end());
  tmp_chapters->RemoveAll();
}

void
kax_reader_c::handle_tags(mm_io_c *io,
                          libebml::EbmlElement *l0,
                          int64_t pos) {
  if (has_deferred_element_been_processed(dl1t_tags, pos))
    return;

  io->save_pos(pos);
  mtx::at_scope_exit_c restore([io]() { io->restore_pos(); });

  int upper_lvl_el = 0;
  std::shared_ptr<libebml::EbmlElement> l1(m_es->FindNextElement(EBML_CONTEXT(l0), upper_lvl_el, 0xFFFFFFFFL, true));
  auto tags = dynamic_cast<libmatroska::KaxTags *>(l1.get());

  if (!tags)
    return;

  libebml::EbmlElement *element_found{};
  upper_lvl_el = 0;

  tags->Read(*m_es, EBML_CLASS_CONTEXT(libmatroska::KaxTags), upper_lvl_el, element_found, true);
  if (!found_in(*tags, element_found))
    delete element_found;

  if (m_regenerate_track_uids)
    remap_uids<libmatroska::KaxTagTrackUID>(*tags, m_track_uid_mapping);

  while (tags->ListSize() > 0) {
    if (!is_type<libmatroska::KaxTag>((*tags)[0])) {
      delete (*tags)[0];
      tags->Remove(0);
      continue;
    }

    bool delete_tag = true;
    bool is_global  = true;
    auto tag        = static_cast<libmatroska::KaxTag *>((*tags)[0]);
    auto target     = find_child<libmatroska::KaxTagTargets>(tag);

    if (target) {
      auto tuid = find_child<libmatroska::KaxTagTrackUID>(target);

      if (tuid) {
        is_global  = false;
        auto track = find_track_by_uid(tuid->GetValue());

        if (track) {
          bool contains_tag = false;

          size_t i;
          for (i = 0; i < tag->ListSize(); i++)
            if (dynamic_cast<libmatroska::KaxTagSimple *>((*tag)[i])) {
              contains_tag = true;
              break;
            }

          if (contains_tag) {
            if (!track->tags)
              track->tags.reset(new libmatroska::KaxTags);
            track->tags->PushElement(*tag);

            delete_tag = false;
          }
        }
      }
    }

    if (is_global) {
      if (!m_tags)
        m_tags = std::shared_ptr<libmatroska::KaxTags>(new libmatroska::KaxTags);
      m_tags->PushElement(*tag);

    } else if (delete_tag)
      delete tag;

    tags->Remove(0);
  }
}

void
kax_reader_c::handle_track_statistics_tags() {
  auto remove_track_statistics_tags = !mtx::hacks::is_engaged(mtx::hacks::KEEP_TRACK_STATISTICS_TAGS) && !g_identifying;

  for (auto const &track : m_tracks) {
    if (!m_ti.m_track_tags.none())
      track->get_source_id_from_track_statistics_tags();

    if (remove_track_statistics_tags)
      track->discard_track_statistics_tags();
  }
}

void
kax_reader_c::handle_cues(mm_io_c *io,
                          libebml::EbmlElement *l0,
                          int64_t pos) {
  if (has_deferred_element_been_processed(dl1t_cues, pos))
    return;

  io->save_pos(pos);
  mtx::at_scope_exit_c restore([io]() { io->restore_pos(); });

  int upper_lvl_el = 0;
  std::shared_ptr<libebml::EbmlElement> l1(m_es->FindNextElement(EBML_CONTEXT(l0), upper_lvl_el, 0xFFFFFFFFL, true));
  auto cues = dynamic_cast<libmatroska::KaxCues *>(l1.get());

  if (!cues)
    return;

  libebml::EbmlElement *element_found{};
  upper_lvl_el = 0;

  cues->Read(*m_es, EBML_CLASS_CONTEXT(libmatroska::KaxCues), upper_lvl_el, element_found, true);
  if (!found_in(*cues, element_found))
    delete element_found;

  std::unordered_map<uint64_t, kax_track_t *> tracks_by_number;

  for (auto const &track : m_tracks)
    tracks_by_number[track->track_number] = track.get();

  auto not_found = tracks_by_number.end();

  for (auto const &cues_child : *cues) {
    if (!is_type<libmatroska::KaxCuePoint>(cues_child))
      continue;

    auto const &cue_point = *static_cast<libmatroska::KaxCuePoint *>(cues_child);

    for (auto const &point_child : cue_point) {
      if (!is_type<libmatroska::KaxCueTrackPositions>(point_child))
        continue;

      auto cue_track = find_child<libmatroska::KaxCueTrack>(static_cast<libmatroska::KaxCueTrackPositions *>(point_child));
      if (!cue_track)
        continue;

      auto itr = tracks_by_number.find(static_cast<libmatroska::KaxCueTrack *>(cue_track)->GetValue());
      if (itr != not_found)
        itr->second->num_cue_points++;
    }
  }
}

void
kax_reader_c::read_headers_info(mm_io_c *io,
                                libebml::EbmlElement *l0,
                                int64_t pos) {
  // General info about this Matroska file
  if (has_deferred_element_been_processed(dl1t_info, pos))
    return;

  io->save_pos(pos);
  mtx::at_scope_exit_c restore([io]() { io->restore_pos(); });

  int upper_lvl_el = 0;
  std::shared_ptr<libebml::EbmlElement> l1(m_es->FindNextElement(EBML_CONTEXT(l0), upper_lvl_el, 0xFFFFFFFFL, true));
  auto info = dynamic_cast<libmatroska::KaxInfo *>(l1.get());

  if (!info)
    return;

  libebml::EbmlElement *element_found{};
  upper_lvl_el = 0;

  info->Read(*m_es, EBML_CLASS_CONTEXT(libmatroska::KaxInfo), upper_lvl_el, element_found, true);
  if (!found_in(*info, element_found))
    delete element_found;

  m_tc_scale          = find_child_value<kax_timestamp_scale_c, uint64_t>(info, 1000000);
  m_segment_duration  = std::llround(find_child_value<libmatroska::KaxDuration>(info) * m_tc_scale);
  m_ti.m_title        = to_utf8(find_child_value<libmatroska::KaxTitle>(info));
  auto muxing_date    = find_child<libmatroska::KaxDateUTC>(info);
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
  auto km_writing_app = find_child<libmatroska::KaxWritingApp>(info);
  if (km_writing_app) {
    read_headers_info_writing_app(km_writing_app);

    // Workaround: HandBrake and other tools always assign sequential
    // numbers starting at 1 to UID attributes. This is a problem when
    // appending two files created by such a tool that contain chapters
    // as both files contain chapters with the same UIDs and mkvmerge
    // thinks those should be merged. So ignore the chapter UIDs for
    // files that aren't created by known-good applications.
    if (!Q(m_writing_app).contains(QRegularExpression{"^(?:mkvmerge|no_variable_data)", QRegularExpression::CaseInsensitiveOption}))
      m_regenerate_chapter_uids = true;

    if (!g_identifying && (mtx::string::to_lower_ascii(m_writing_app).find("makemkv") != std::string::npos))
      m_regenerate_track_uids = true;
  }

  auto km_muxing_app = find_child<libmatroska::KaxMuxingApp>(info);
  if (km_muxing_app) {
    m_muxing_app = km_muxing_app->GetValueUTF8();

    // DirectShow Muxer workaround: Gabest's DirectShow muxer
    // writes wrong references (off by 1ms). So let the cluster
    // helper be a bit more imprecise in what it accepts when
    // looking for referenced packets.
    if (m_muxing_app == "DirectShow Matroska Muxer")
      m_reference_timestamp_tolerance = 1000000;
  }

  m_segment_uid          = find_child_value<libmatroska::KaxSegmentUID>(info);
  m_next_segment_uid     = find_child_value<libmatroska::KaxNextUID>(info);
  m_previous_segment_uid = find_child_value<libmatroska::KaxPrevUID>(info);
}

void
kax_reader_c::read_headers_info_writing_app(libmatroska::KaxWritingApp *&km_writing_app) {
  size_t idx;

  std::string s = km_writing_app->GetValueUTF8();
  mtx::string::strip(s);

  m_raw_writing_app = s;

  if (balg::istarts_with(s, "avi-mux gui"))
    s.replace(0, strlen("avi-mux gui"), "avimuxgui");

  auto parts = mtx::string::split(s.c_str(), " ", 3);
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

    auto ver_parts = mtx::string::split(s.c_str(), ".");
    for (idx = ver_parts.size(); idx < 4; idx++)
      ver_parts.push_back("0");

    bool failed     = false;
    m_writing_app_ver = 0;

    for (idx = 0; idx < 4; idx++) {
      int num;

      if (!mtx::string::parse_number(ver_parts[idx], num) || (0 > num) || (255 < num)) {
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
                                       libmatroska::KaxTrackAudio *ktaudio) {
  track->a_sfreq    = find_child_value<libmatroska::KaxAudioSamplingFreq>(ktaudio, 8000.0);
  track->a_osfreq   = find_child_value<libmatroska::KaxAudioOutputSamplingFreq>(ktaudio);
  track->a_channels = find_child_value<libmatroska::KaxAudioChannels>(ktaudio, 1);
  track->a_bps      = find_child_value<libmatroska::KaxAudioBitDepth>(ktaudio);
  track->a_emphasis = find_optional_child_value<libmatroska::KaxEmphasis>(ktaudio);
}

void
kax_reader_c::read_headers_track_video(kax_track_t *track,
                                       libmatroska::KaxTrackVideo *ktvideo) {
  track->v_width        = find_child_value<libmatroska::KaxVideoPixelWidth>(ktvideo);
  track->v_height       = find_child_value<libmatroska::KaxVideoPixelHeight>(ktvideo);
  track->v_dwidth       = find_child_value<libmatroska::KaxVideoDisplayWidth>(ktvideo,  track->v_width);
  track->v_dheight      = find_child_value<libmatroska::KaxVideoDisplayHeight>(ktvideo, track->v_height);
  track->v_dunit        = find_child_value<libmatroska::KaxVideoDisplayUnit>(ktvideo,   generic_packetizer_c::ddu_pixels);

  track->v_pcleft       = find_child_value<libmatroska::KaxVideoPixelCropLeft>(ktvideo);
  track->v_pcright      = find_child_value<libmatroska::KaxVideoPixelCropRight>(ktvideo);
  track->v_pctop        = find_child_value<libmatroska::KaxVideoPixelCropTop>(ktvideo);
  track->v_pcbottom     = find_child_value<libmatroska::KaxVideoPixelCropBottom>(ktvideo);

  track->v_alpha_mode   = find_optional_child_bool_value<libmatroska::KaxVideoAlphaMode>(ktvideo);

  auto color_space      = find_child<libmatroska::KaxVideoColourSpace>(*ktvideo);
  if (color_space)
    track->v_color_space = memory_c::clone(color_space->GetBuffer(), color_space->GetSize());

  auto color = find_child<libmatroska::KaxVideoColour>(*ktvideo);

  if (color) {
    track->v_color_matrix          = find_optional_child_value<libmatroska::KaxVideoColourMatrix>(color);
    track->v_bits_per_channel      = find_optional_child_value<libmatroska::KaxVideoBitsPerChannel>(color);
    track->v_chroma_subsample.hori = find_optional_child_value<libmatroska::KaxVideoChromaSubsampHorz>(color);
    track->v_chroma_subsample.vert = find_optional_child_value<libmatroska::KaxVideoChromaSubsampVert>(color);
    track->v_cb_subsample.hori     = find_optional_child_value<libmatroska::KaxVideoCbSubsampHorz>(color);
    track->v_cb_subsample.vert     = find_optional_child_value<libmatroska::KaxVideoCbSubsampVert>(color);
    track->v_chroma_siting.hori    = find_optional_child_value<libmatroska::KaxVideoChromaSitHorz>(color);
    track->v_chroma_siting.vert    = find_optional_child_value<libmatroska::KaxVideoChromaSitVert>(color);
    track->v_color_range           = find_optional_child_value<libmatroska::KaxVideoColourRange>(color);
    track->v_transfer_character    = find_optional_child_value<libmatroska::KaxVideoColourTransferCharacter>(color);
    track->v_color_primaries       = find_optional_child_value<libmatroska::KaxVideoColourPrimaries>(color);
    track->v_max_cll               = find_optional_child_value<libmatroska::KaxVideoColourMaxCLL>(color);
    track->v_max_fall              = find_optional_child_value<libmatroska::KaxVideoColourMaxFALL>(color);

    auto color_meta                = find_child<libmatroska::KaxVideoColourMasterMeta>(*color);

    if (color_meta) {
      track->v_chroma_coordinates.red_x     = find_optional_child_value<libmatroska::KaxVideoRChromaX>(color_meta);
      track->v_chroma_coordinates.red_y     = find_optional_child_value<libmatroska::KaxVideoRChromaY>(color_meta);
      track->v_chroma_coordinates.green_x   = find_optional_child_value<libmatroska::KaxVideoGChromaX>(color_meta);
      track->v_chroma_coordinates.green_y   = find_optional_child_value<libmatroska::KaxVideoGChromaY>(color_meta);
      track->v_chroma_coordinates.blue_x    = find_optional_child_value<libmatroska::KaxVideoBChromaX>(color_meta);
      track->v_chroma_coordinates.blue_y    = find_optional_child_value<libmatroska::KaxVideoBChromaY>(color_meta);
      track->v_white_color_coordinates.hori = find_optional_child_value<libmatroska::KaxVideoWhitePointChromaX>(color_meta);
      track->v_white_color_coordinates.vert = find_optional_child_value<libmatroska::KaxVideoWhitePointChromaY>(color_meta);
      track->v_max_luminance                = find_optional_child_value<libmatroska::KaxVideoLuminanceMax>(color_meta);
      track->v_min_luminance                = find_optional_child_value<libmatroska::KaxVideoLuminanceMin>(color_meta);
    }
  }

  auto projection = find_child<libmatroska::KaxVideoProjection>(*ktvideo);

  if (projection) {
    track->v_projection_type       = find_optional_child_value<libmatroska::KaxVideoProjectionType>(projection);
    track->v_projection_pose_yaw   = find_optional_child_value<libmatroska::KaxVideoProjectionPoseYaw>(projection);
    track->v_projection_pose_pitch = find_optional_child_value<libmatroska::KaxVideoProjectionPosePitch>(projection);
    track->v_projection_pose_roll  = find_optional_child_value<libmatroska::KaxVideoProjectionPoseRoll>(projection);

    auto kprojection_private = find_child<libmatroska::KaxVideoProjectionPrivate>(projection);
    if (kprojection_private)
      track->v_projection_private = memory_c::clone(kprojection_private->GetBuffer(), kprojection_private->GetSize());
  }

  track->v_field_order  = find_optional_child_value<libmatroska::KaxVideoFieldOrder>(ktvideo);
  track->v_stereo_mode  = find_child_value<libmatroska::KaxVideoStereoMode, stereo_mode_c::mode>(ktvideo, stereo_mode_c::unspecified);

  // For older files.
  auto frame_rate       = find_child_value<libmatroska::KaxVideoFrameRate>(ktvideo);
  track->v_display_unit = find_child_value<libmatroska::KaxVideoDisplayUnit>(ktvideo);

  if (0 < frame_rate)
    track->default_duration = static_cast<int64_t>(1'000'000'000 / frame_rate);

  if (!track->v_width)
    mxerror(Y("matroska_reader: Pixel width is missing.\n"));
  if (!track->v_height)
    mxerror(Y("matroska_reader: Pixel height is missing.\n"));

  track->fix_display_dimension_parameters();
}

void
kax_reader_c::read_headers_tracks(mm_io_c *io,
                                  libebml::EbmlElement *l0,
                                  int64_t position) {
  // Yep, we've found our libmatroska::KaxTracks element. Now find all m_tracks
  // contained in this segment.
  if (has_deferred_element_been_processed(dl1t_tracks, position))
    return;

  int upper_lvl_el = 0;
  io->save_pos(position);
  auto l1 = std::shared_ptr<libebml::EbmlElement>(m_es->FindNextElement(EBML_CONTEXT(l0), upper_lvl_el, 0xFFFFFFFFL, true));

  if (!l1 || !is_type<libmatroska::KaxTracks>(*l1)) {
    io->restore_pos();

    return;
  }

  libebml::EbmlElement *element_found{};
  upper_lvl_el = 0;

  l1->Read(*m_es, EBML_CLASS_CONTEXT(libmatroska::KaxTracks), upper_lvl_el, element_found, true);
  if (!found_in(*l1, element_found))
    delete element_found;

  for (auto *ktentry = find_child<libmatroska::KaxTrackEntry>(static_cast<libebml::EbmlMaster &>(*l1)); ktentry; ktentry = FindNextChild(static_cast<libebml::EbmlMaster &>(*l1), *ktentry)) {
    // We actually found a track entry :) We're happy now.
    auto track  = std::make_shared<kax_track_t>();
    track->tnum = m_tracks.size();

    auto ktnum = find_child<libmatroska::KaxTrackNumber>(ktentry);
    if (!ktnum) {
      mxdebug_if(m_debug_track_headers, fmt::format("matroska_reader: track ID {} is missing its track number.\n", track->tnum));
      continue;
    }

    track->track_number = ktnum->GetValue();
    if (find_track_by_num(track->track_number, track.get())) {
      mxdebug_if(m_debug_track_headers, fmt::format("matroska_reader: track number {} is already in use.\n", track->track_number));
      m_known_bad_track_numbers[track->track_number] = true;
      continue;
    }

    auto ktuid = find_child<libmatroska::KaxTrackUID>(ktentry);
    if (!ktuid)
      mxwarn_fn(m_ti.m_fname,
                fmt::format(FY("Track {0} is missing its track UID element which is required to be present by the Matroska specification. If the file contains tags then those tags might be broken.\n"),
                            track->tnum));
    else {
      track->track_uid = ktuid->GetValue();

      if (m_regenerate_track_uids) {
        uint64_t new_id{};
        do {
          new_id = random_c::generate_64bits();
        } while ((new_id == 0) || (m_track_uid_mapping.find(new_id) != m_track_uid_mapping.end()));

        m_track_uid_mapping.insert({ track->track_uid, new_id });
        track->track_uid = new_id;
      }
    }

    auto kttype = find_child<libmatroska::KaxTrackType>(ktentry);
    if (!kttype) {
      mxdebug_if(m_debug_track_headers, fmt::format("matroska_reader: track number {} is missing the track type.\n", track->track_number));
      m_known_bad_track_numbers[track->track_number] = true;
      continue;
    }

    auto track_type = kttype->GetValue();
    track->type     = track_type == track_audio    ? 'a'
                    : track_type == track_video    ? 'v'
                    : track_type == track_subtitle ? 's'
                    :                                '?';

    auto ktaudio = find_child<libmatroska::KaxTrackAudio>(ktentry);
    if (ktaudio)
      read_headers_track_audio(track.get(), ktaudio);

    auto ktvideo = find_child<libmatroska::KaxTrackVideo>(ktentry);
    if (ktvideo)
      read_headers_track_video(track.get(), ktvideo);

    auto kcodecpriv = find_child<libmatroska::KaxCodecPrivate>(ktentry);
    if (kcodecpriv)
      track->private_data = memory_c::clone(kcodecpriv->GetBuffer(), kcodecpriv->GetSize());

    track->codec_id               = find_child_value<libmatroska::KaxCodecID>(ktentry);
    track->codec_name             = to_utf8(find_child_value<libmatroska::KaxCodecName>(ktentry));
    track->track_name             = to_utf8(find_child_value<libmatroska::KaxTrackName>(ktentry));
    track->language               = mtx::bcp47::language_c::parse(find_child_value<libmatroska::KaxTrackLanguage, std::string>(ktentry, "eng"));
    track->language_ietf          = mtx::bcp47::language_c::parse(find_child_value<libmatroska::KaxLanguageIETF, std::string>(ktentry, {}));
    track->default_duration       = find_child_value<libmatroska::KaxTrackDefaultDuration>(ktentry, track->default_duration);
    track->default_track          = find_child_value<libmatroska::KaxTrackFlagDefault, bool>(ktentry, true);
    track->forced_track           = find_child_value<libmatroska::KaxTrackFlagForced>(ktentry);
    track->enabled_track          = find_child_value<libmatroska::KaxTrackFlagEnabled, bool>(ktentry, true);
    track->lacing_flag            = find_child_value<libmatroska::KaxTrackFlagLacing>(ktentry);
    track->max_blockadd_id        = find_child_value<libmatroska::KaxMaxBlockAdditionID>(ktentry);
    track->hearing_impaired_flag  = find_optional_child_bool_value<libmatroska::KaxFlagHearingImpaired>(ktentry);
    track->visual_impaired_flag   = find_optional_child_bool_value<libmatroska::KaxFlagVisualImpaired>(ktentry);
    track->text_descriptions_flag = find_optional_child_bool_value<libmatroska::KaxFlagTextDescriptions>(ktentry);
    track->original_flag          = find_optional_child_bool_value<libmatroska::KaxFlagOriginal>(ktentry);
    track->commentary_flag        = find_optional_child_bool_value<libmatroska::KaxFlagCommentary>(ktentry);

    auto kax_seek_pre_roll        = find_child<libmatroska::KaxSeekPreRoll>(ktentry);
    auto kax_codec_delay          = find_child<libmatroska::KaxCodecDelay>(ktentry);

    if (kax_seek_pre_roll)
      track->seek_pre_roll = timestamp_c::ns(kax_seek_pre_roll->GetValue());
    if (kax_codec_delay)
      track->codec_delay   = timestamp_c::ns(kax_codec_delay->GetValue());

    for (auto const &child : *ktentry) {
      auto kmapping = dynamic_cast<libmatroska::KaxBlockAdditionMapping *>(child);
      if (!kmapping)
        continue;

      block_addition_mapping_t mapping;

      mapping.id_name       = find_child_value<libmatroska::KaxBlockAddIDName>(kmapping);
      mapping.id_type       = find_optional_child_value<libmatroska::KaxBlockAddIDType>(kmapping);
      mapping.id_value      = find_optional_child_value<libmatroska::KaxBlockAddIDValue>(kmapping);
      mapping.id_extra_data = find_child_value<libmatroska::KaxBlockAddIDExtraData>(kmapping);

      if (mapping.is_valid())
        track->block_addition_mappings.push_back(mapping);
    }

    if (track->codec_id.empty()) {
      mxdebug_if(m_debug_track_headers, fmt::format("matroska_reader: track number {} is missing the CodecID.\n", track->track_number));
      m_known_bad_track_numbers[track->track_number] = true;
      continue;
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
    if (!track->language.is_valid())
      track->language = mtx::bcp47::language_c::parse("und");

    track->effective_language = track->language_ietf.is_valid() ? track->language_ietf : track->language;

    track->content_decoder.initialize(*ktentry);

    m_tracks.push_back(track);
  } // while (ktentry)

  io->restore_pos();
}

void
kax_reader_c::handle_seek_head(mm_io_c *io,
                               libebml::EbmlElement *l0,
                               int64_t pos) {
  if (has_deferred_element_been_processed(dl1t_seek_head, pos))
    return;

  std::vector<int64_t> next_seek_head_positions;
  mtx::at_scope_exit_c restore([io]() { io->restore_pos(); });

  try {
    io->save_pos(pos);

    int upper_lvl_el = 0;
    std::shared_ptr<libebml::EbmlElement> l1(m_es->FindNextElement(EBML_CONTEXT(l0), upper_lvl_el, 0xFFFFFFFFL, true));
    auto *seek_head = dynamic_cast<libmatroska::KaxSeekHead *>(l1.get());

    if (!seek_head)
      return;

    libebml::EbmlElement *element_found = nullptr;
    upper_lvl_el               = 0;

    seek_head->Read(*m_es, EBML_CLASS_CONTEXT(libmatroska::KaxSeekHead), upper_lvl_el, element_found, true);
    if (!found_in(*seek_head, element_found))
      delete element_found;

    for (auto l2 : *seek_head) {
      if (!is_type<libmatroska::KaxSeek>(l2))
        continue;

      auto &seek        = *static_cast<libmatroska::KaxSeek *>(l2);
      auto new_seek_pos = find_child_value<libmatroska::KaxSeekPosition, int64_t>(seek, -1);

      if (-1 == new_seek_pos)
        continue;

      auto *k_id = find_child<libmatroska::KaxSeekID>(seek);
      if (!k_id)
        continue;

      if (k_id->GetSize() > 4)
        continue;

      auto id = create_ebml_id_from(*k_id);

      deferred_l1_type_e type = is_type<libmatroska::KaxAttachments>(id) ? dl1t_attachments
        :                       is_type<libmatroska::KaxChapters>(id)    ? dl1t_chapters
        :                       is_type<libmatroska::KaxTags>(id)        ? dl1t_tags
        :                       is_type<libmatroska::KaxTracks>(id)      ? dl1t_tracks
        :                       is_type<libmatroska::KaxSeekHead>(id)    ? dl1t_seek_head
        :                       is_type<libmatroska::KaxInfo>(id)        ? dl1t_info
        :                       is_type<libmatroska::KaxCues>(id)        ? dl1t_cues
        :                                                                  dl1t_unknown;

      if (dl1t_unknown == type)
        continue;

      new_seek_pos = static_cast<libmatroska::KaxSegment *>(l0)->GetGlobalPosition(new_seek_pos);

      if (dl1t_seek_head == type)
        next_seek_head_positions.push_back(new_seek_pos);
      else
        m_deferred_l1_positions[type].push_back(new_seek_pos);
    }

  } catch (...) {
    return;
  }

  for (auto new_seek_head_pos : next_seek_head_positions)
    handle_seek_head(io, l0, new_seek_head_pos);
}

void
kax_reader_c::read_headers() {
  if (!read_headers_internal())
    throw mtx::input::header_parsing_x();

  determine_minimum_timestamps();
  determine_global_timestamp_offset_to_apply();
  adjust_chapter_timestamps();

  show_demuxer_info();
}

void
kax_reader_c::adjust_chapter_timestamps() {
  if (!m_chapters)
    return;

  auto const &sync = mtx::includes(m_ti.m_timestamp_syncs, track_info_c::chapter_track_id) ? m_ti.m_timestamp_syncs[track_info_c::chapter_track_id]
                   : mtx::includes(m_ti.m_timestamp_syncs, track_info_c::all_tracks_id)    ? m_ti.m_timestamp_syncs[track_info_c::all_tracks_id]
                   :                                                                         timestamp_sync_t{};

  mtx::chapters::adjust_timestamps(*m_chapters, -m_global_timestamp_offset);
  mtx::chapters::adjust_timestamps(*m_chapters, sync.displacement, sync.factor);
}

void
kax_reader_c::find_level1_elements_via_analyzer() {
  try {
    auto start_pos = m_in->get_size() - std::min<int64_t>(m_in->get_size(), 5 * 1024 * 1024);
    auto analyzer  = std::make_shared<kax_analyzer_c>(m_in);
    auto ok        = analyzer
      ->set_parse_mode(kax_analyzer_c::parse_mode_full)
      .set_open_mode(libebml::MODE_READ)
      .set_parser_start_position(start_pos)
      .process();

    if (!ok)
      return;

    analyzer->with_elements(EBML_ID(libmatroska::KaxInfo),        [this](kax_analyzer_data_c const &data) { m_deferred_l1_positions[dl1t_info       ].push_back(data.m_pos); });
    analyzer->with_elements(EBML_ID(libmatroska::KaxTracks),      [this](kax_analyzer_data_c const &data) { m_deferred_l1_positions[dl1t_tracks     ].push_back(data.m_pos); });
    analyzer->with_elements(EBML_ID(libmatroska::KaxAttachments), [this](kax_analyzer_data_c const &data) { m_deferred_l1_positions[dl1t_attachments].push_back(data.m_pos); });
    analyzer->with_elements(EBML_ID(libmatroska::KaxChapters),    [this](kax_analyzer_data_c const &data) { m_deferred_l1_positions[dl1t_chapters   ].push_back(data.m_pos); });
    analyzer->with_elements(EBML_ID(libmatroska::KaxTags),        [this](kax_analyzer_data_c const &data) { m_deferred_l1_positions[dl1t_tags       ].push_back(data.m_pos); });

  } catch (...) {
  }
}

void
kax_reader_c::read_deferred_level1_elements(libmatroska::KaxSegment &segment) {
  for (auto position : m_deferred_l1_positions[dl1t_info])
    read_headers_info(m_in.get(), &segment, position);

  for (auto position : m_deferred_l1_positions[dl1t_tracks])
    read_headers_tracks(m_in.get(), &segment, position);

  if (!m_ti.m_attach_mode_list.none())
    for (auto position : m_deferred_l1_positions[dl1t_attachments])
      handle_attachments(m_in.get(), &segment, position);

  for (auto position : m_deferred_l1_positions[dl1t_tags])
    handle_tags(m_in.get(), &segment, position);

  if (!m_ti.m_no_chapters)
    for (auto position : m_deferred_l1_positions[dl1t_chapters])
      handle_chapters(m_in.get(), &segment, position);

  for (auto position : m_deferred_l1_positions[dl1t_cues])
    handle_cues(m_in.get(), &segment, position);

  handle_track_statistics_tags();

  if (!m_ti.m_no_global_tags)
    process_global_tags();
}

bool
kax_reader_c::read_headers_internal() {
  // Elements for different levels

  if (!g_identifying)
    m_regenerate_track_uids = m_ti.m_regenerate_track_uids;

  auto cluster = std::shared_ptr<libmatroska::KaxCluster>{};
  try {
    m_es      = std::shared_ptr<libebml::EbmlStream>(new libebml::EbmlStream(*m_in));
    m_in_file = std::make_shared<kax_file_c>(*m_in);

    m_in_file->enable_reporting(!g_identifying);

    // Find the libebml::EbmlHead element. Must be the first one.
    auto l0 = std::shared_ptr<libebml::EbmlElement>(m_es->FindNextID(EBML_INFO(libebml::EbmlHead), 0xFFFFFFFFFFFFFFFFLL));
    if (!l0 || !dynamic_cast<libebml::EbmlHead *>(l0.get())) {
      mxwarn(Y("matroska_reader: no EBML head found.\n"));
      return false;
    }

    auto &head = static_cast<libebml::EbmlHead &>(*l0);
    int upper_lvl_el{};
    libebml::EbmlElement *element_found{};

    head.Read(*m_es, EBML_CONTEXT(&head), upper_lvl_el, element_found, true);
    delete element_found;

    m_is_webm = find_child_value<libebml::EDocType>(head) == "webm";

    l0->SkipData(*m_es, EBML_CONTEXT(l0.get()));

    while (true) {
      // Next element must be a segment
      l0.reset(m_es->FindNextID(EBML_INFO(libmatroska::KaxSegment), 0xFFFFFFFFFFFFFFFFLL));
      if (!l0) {
        if (verbose)
          mxwarn(Y("matroska_reader: No segment found.\n"));
        return false;
      }

      if (is_type<libmatroska::KaxSegment>(*l0))
        break;

      if (is_type<libebml::EbmlCrc32>(*l0) || is_type<libebml::EbmlVoid>(*l0) || is_dummy(*l0)) {
        l0->SkipData(*m_es, EBML_CONTEXT(l0.get()));
        continue;
      }

      if (verbose)
        mxwarn(Y("matroska_reader: No segment found.\n"));
      return false;
    }

    m_in_file->set_segment_end(static_cast<libmatroska::KaxSegment &>(*l0));

    // We've got our segment, so let's find the m_tracks
    m_tc_scale = TIMESTAMP_SCALE;

    while (m_in->getFilePointer() < m_in_file->get_segment_end()) {
      auto l1 = m_in_file->read_next_level1_element();
      if (!l1)
        break;

      if (is_type<libmatroska::KaxInfo>(*l1))
        m_deferred_l1_positions[dl1t_info].push_back(l1->GetElementPosition());

      else if (is_type<libmatroska::KaxTracks>(*l1))
        m_deferred_l1_positions[dl1t_tracks].push_back(l1->GetElementPosition());

      else if (is_type<libmatroska::KaxAttachments>(*l1))
        m_deferred_l1_positions[dl1t_attachments].push_back(l1->GetElementPosition());

      else if (is_type<libmatroska::KaxChapters>(*l1))
        m_deferred_l1_positions[dl1t_chapters].push_back(l1->GetElementPosition());

      else if (is_type<libmatroska::KaxTags>(*l1))
        m_deferred_l1_positions[dl1t_tags].push_back(l1->GetElementPosition());

      else if (is_type<libmatroska::KaxSeekHead>(*l1))
        handle_seek_head(m_in.get(), l0.get(), l1->GetElementPosition());

      else if (is_type<libmatroska::KaxCluster>(*l1))
        cluster = std::static_pointer_cast<libmatroska::KaxCluster>(l1);

      else
        l1->SkipData(*m_es, EBML_CONTEXT(l1.get()));

      if (cluster)              // we've found the first cluster, so get out
        break;

      auto in_parent = !l0->IsFiniteSize() || (m_in->getFilePointer() < (l0->GetElementPosition() + get_head_size(*l0) + l0->GetSize()));

      if (!in_parent)
        break;

      l1->SkipData(*m_es, EBML_CONTEXT(l1.get()));

    } // while (l1)

    if (m_handled_l1_positions[dl1t_seek_head].empty() || debugging_c::requested("kax_reader_use_analyzer"))
      find_level1_elements_via_analyzer();

    read_deferred_level1_elements(static_cast<libmatroska::KaxSegment &>(*l0));

  } catch (...) {
    mxwarn(fmt::format("{0} {1} {2}\n",
                       fmt::format(FY("{0}: an unknown exception occurred."), "kax_reader_c::read_headers_internal()"),
                       Y("This usually indicates a damaged file structure."), Y("The file will not be processed further.")));
  }

  auto cluster_pos = cluster ? cluster->GetElementPosition() : m_in->get_size();
  m_in->setFilePointer(cluster_pos);

  verify_tracks();

  m_in->setFilePointer(cluster_pos);

  return true;
}

void
kax_reader_c::process_global_tags() {
  if (!m_tags || g_identifying)
    return;

  for (auto tag : *m_tags)
    add_tags(static_cast<libmatroska::KaxTag &>(*tag));

  m_tags->RemoveAll();
}

void
kax_reader_c::init_passthrough_packetizer(kax_track_t *t,
                                          track_info_c &nti) {
  mxinfo_tid(m_ti.m_fname, t->tnum, fmt::format(FY("Using the generic output module for track type '{0}'.\n"), map_track_type_string(t->type)));

  auto packetizer                 = new passthrough_packetizer_c{this, nti, map_track_type(t->type)};
  t->ptzr                         = add_packetizer(packetizer);
  t->ptzr_ptr                     = packetizer;
  t->passthrough                  = true;
  m_ptzr_to_track_map[packetizer] = t;

  packetizer->set_codec_id(t->codec_id);
  packetizer->set_codec_private(t->private_data);
  packetizer->set_codec_name(t->codec_name);

  if (t->default_duration)
    packetizer->set_track_default_duration(t->default_duration);
  if (t->seek_pre_roll.valid())
    packetizer->set_track_seek_pre_roll(t->seek_pre_roll);

  t->handle_packetizer_block_addition_mapping();

  if ('v' == t->type) {
    packetizer->set_video_pixel_dimensions(t->v_width, t->v_height);

    t->handle_packetizer_display_dimensions();
    t->handle_packetizer_pixel_cropping();
    t->handle_packetizer_color();
    t->handle_packetizer_field_order();
    t->handle_packetizer_stereo_mode();
    t->handle_packetizer_alpha_mode();

    if (CUE_STRATEGY_UNSPECIFIED == packetizer->get_cue_creation())
      packetizer->set_cue_creation(CUE_STRATEGY_IFRAMES);

  } else if ('a' == t->type) {
    packetizer->set_audio_sampling_freq(t->a_sfreq);
    packetizer->set_audio_channels(t->a_channels);
    if (0 != t->a_bps)
      packetizer->set_audio_bit_depth(t->a_bps);
    if (0.0 != t->a_osfreq)
      packetizer->set_audio_output_sampling_freq(t->a_osfreq);

  } else {
    // Nothing to do for subs, I guess.
  }

}

void
kax_reader_c::set_packetizer_headers(kax_track_t *t) {
  if (m_appending)
    return;

  ptzr(t->ptzr).set_track_default_flag(t->default_track, option_source_e::container);

  if (t->forced_track)
    ptzr(t->ptzr).set_track_forced_flag(true, option_source_e::container);

  if (t->hearing_impaired_flag.has_value())
    ptzr(t->ptzr).set_hearing_impaired_flag(*t->hearing_impaired_flag, option_source_e::container);

  if (t->visual_impaired_flag.has_value())
    ptzr(t->ptzr).set_visual_impaired_flag(*t->visual_impaired_flag, option_source_e::container);

  if (t->text_descriptions_flag.has_value())
    ptzr(t->ptzr).set_text_descriptions_flag(*t->text_descriptions_flag, option_source_e::container);

  if (t->original_flag.has_value())
    ptzr(t->ptzr).set_original_flag(*t->original_flag, option_source_e::container);

  if (t->commentary_flag.has_value())
    ptzr(t->ptzr).set_commentary_flag(*t->commentary_flag, option_source_e::container);

  ptzr(t->ptzr).set_track_enabled_flag(static_cast<bool>(t->enabled_track), option_source_e::container);

  if ((0 != t->track_uid) && !ptzr(t->ptzr).set_uid(t->track_uid))
    mxwarn_fn(m_ti.m_fname, fmt::format(FY("Could not keep a track's UID {0} because it is already allocated for another track. A new random UID will be allocated automatically.\n"), t->track_uid));

  ptzr(t->ptzr).set_codec_name(t->codec_name);
  ptzr(t->ptzr).set_source_id(t->source_id);

  if ((t->type == 'a') && (0 != t->a_bps))
    ptzr(t->ptzr).set_audio_bit_depth(t->a_bps);

  if (t->a_emphasis)
    ptzr(t->ptzr).set_audio_emphasis(static_cast<audio_emphasis_c::mode_e>(*t->a_emphasis), option_source_e::container);

  t->handle_packetizer_block_addition_mapping();
}

void
kax_reader_c::create_video_packetizer(kax_track_t *t,
                                      track_info_c &nti) {
  if (t->codec.is(codec_c::type_e::V_MPEG4_P10) && t->ms_compat && !mtx::hacks::is_engaged(mtx::hacks::ALLOW_AVC_IN_VFW_MODE))
    create_avc_es_video_packetizer(t, nti);

  else if (t->codec.is(codec_c::type_e::V_MPEG12)) {
    int version = t->codec_id[6] - '0';
    set_track_packetizer(t, new mpeg1_2_video_packetizer_c(this, nti, version, t->default_duration, t->v_width, t->v_height, t->v_dwidth, t->v_dheight, true));
    show_packetizer_info(t->tnum, *t->ptzr_ptr);

  } else if (t->codec.is(codec_c::type_e::V_MPEGH_P2))
    create_hevc_video_packetizer(t, nti);

  else if (t->codec.is(codec_c::type_e::V_MPEG4_P2)) {
    bool is_native = (t->codec_id == MKV_V_MPEG4_SP) || (t->codec_id == MKV_V_MPEG4_AP) || (t->codec_id == MKV_V_MPEG4_ASP);
    set_track_packetizer(t, new mpeg4_p2_video_packetizer_c(this, nti, t->default_duration, t->v_width, t->v_height, is_native));
    show_packetizer_info(t->tnum, *t->ptzr_ptr);

  } else if (t->codec.is(codec_c::type_e::V_MPEG4_P10))
    create_avc_video_packetizer(t, nti);

  else if (t->codec.is(codec_c::type_e::V_THEORA)) {
    set_track_packetizer(t, new theora_video_packetizer_c(this, nti, t->default_duration, t->v_width, t->v_height));
    show_packetizer_info(t->tnum, *t->ptzr_ptr);

  } else if (t->codec.is(codec_c::type_e::V_DIRAC)) {
    set_track_packetizer(t, new dirac_video_packetizer_c(this, nti));
    show_packetizer_info(t->tnum, *t->ptzr_ptr);

  } else if (t->codec.is(codec_c::type_e::V_AV1))
    create_av1_video_packetizer(t, nti);

  else if (t->codec.is(codec_c::type_e::V_PRORES))
    create_prores_video_packetizer(*t, nti);

  else if (t->codec.is(codec_c::type_e::V_VP8) || t->codec.is(codec_c::type_e::V_VP9)) {
    set_track_packetizer(t, new vpx_video_packetizer_c(this, nti, t->codec.get_type()));
    show_packetizer_info(t->tnum, *t->ptzr_ptr);
    t->handle_packetizer_pixel_dimensions();
    t->handle_packetizer_default_duration();

  } else if (t->codec.is(codec_c::type_e::V_VC1))
    create_vc1_video_packetizer(t, nti);

  else if (t->ms_compat) {
    set_track_packetizer(t, new video_for_windows_packetizer_c(this, nti, t->default_duration, t->v_width, t->v_height));
    show_packetizer_info(t->tnum, *t->ptzr_ptr);

  } else {
    set_track_packetizer(t, new generic_video_packetizer_c(this, nti, t->codec_id.c_str(), t->default_duration, t->v_width, t->v_height));
    show_packetizer_info(t->tnum, *t->ptzr_ptr);
  }

  t->handle_packetizer_display_dimensions();
  t->handle_packetizer_pixel_cropping();
  t->handle_packetizer_color();
  t->handle_packetizer_field_order();
  t->handle_packetizer_stereo_mode();
  t->handle_packetizer_alpha_mode();
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
  int detected_profile     = mtx::aac::PROFILE_MAIN;

  if (!t->ms_compat) {
    if (t->private_data && (2 <= t->private_data->get_size())) {
      auto parsed_audio_config = mtx::aac::parse_audio_specific_config(t->private_data->get_buffer(), t->private_data->get_size());
      if (!parsed_audio_config)
        mxerror_tid(m_ti.m_fname, t->tnum, Y("Malformed AAC codec initialization data found.\n"));

      audio_config           = *parsed_audio_config;
      detected_profile       = audio_config.profile;
      if (audio_config.sbr)
        audio_config.profile = mtx::aac::PROFILE_SBR;

    } else {
      int id = 0, profile = 0;
      if (!mtx::aac::parse_codec_id(t->codec_id, id, profile))
        mxerror_tid(m_ti.m_fname, t->tnum, fmt::format(FY("Malformed codec id '{0}'.\n"), t->codec_id));
      audio_config.profile = profile;
    }

  } else {
    auto parsed_audio_config = mtx::aac::parse_audio_specific_config(t->private_data->get_buffer() + sizeof(alWAVEFORMATEX), t->private_data->get_size() - sizeof(alWAVEFORMATEX));
    if (!parsed_audio_config)
      mxerror_tid(m_ti.m_fname, t->tnum, Y("Malformed AAC codec initialization data found.\n"));

    audio_config           = *parsed_audio_config;
    detected_profile       = audio_config.profile;
    if (audio_config.sbr)
      audio_config.profile = mtx::aac::PROFILE_SBR;
  }

  if ((mtx::includes(m_ti.m_all_aac_is_sbr, t->tnum) &&  m_ti.m_all_aac_is_sbr[t->tnum]) || (mtx::includes(m_ti.m_all_aac_is_sbr, -1) &&  m_ti.m_all_aac_is_sbr[-1]))
    audio_config.profile = mtx::aac::PROFILE_SBR;

  if ((mtx::includes(m_ti.m_all_aac_is_sbr, t->tnum) && !m_ti.m_all_aac_is_sbr[t->tnum]) || (mtx::includes(m_ti.m_all_aac_is_sbr, -1) && !m_ti.m_all_aac_is_sbr[-1]))
    audio_config.profile = detected_profile;

  set_track_packetizer(t, new aac_packetizer_c(this, nti, audio_config, aac_packetizer_c::headerless));
  show_packetizer_info(t->tnum, *t->ptzr_ptr);
}

void
kax_reader_c::create_ac3_audio_packetizer(kax_track_t *t,
                                          track_info_c &nti) {
  unsigned int bsid = t->codec_id == "A_AC3/BSID9"  ?  9
                    : t->codec_id == "A_AC3/BSID10" ? 10
                    : t->codec_id == MKV_A_EAC3     ? 16
                    :                                  0;

  set_track_packetizer(t, new ac3_packetizer_c(this, nti, t->a_sfreq, t->a_channels, bsid));
  show_packetizer_info(t->tnum, *t->ptzr_ptr);
}

void
kax_reader_c::create_alac_audio_packetizer(kax_track_t *t,
                                          track_info_c &nti) {
  set_track_packetizer(t, new alac_packetizer_c(this, nti, t->private_data, t->a_sfreq, t->a_channels));
  show_packetizer_info(t->tnum, *t->ptzr_ptr);
}

void
kax_reader_c::create_dts_audio_packetizer(kax_track_t *t,
                                          track_info_c &nti) {
  set_track_packetizer(t, new dts_packetizer_c(this, nti, t->dts_header));
  show_packetizer_info(t->tnum, *t->ptzr_ptr);
}

#if defined(HAVE_FLAC_FORMAT_H)
void
kax_reader_c::create_flac_audio_packetizer(kax_track_t *t,
                                           track_info_c &nti) {
  nti.m_private_data.reset();

  unsigned int offset = t->ms_compat ? sizeof(alWAVEFORMATEX) : 0u;
  set_track_packetizer(t, new flac_packetizer_c(this, nti, t->private_data->get_buffer() + offset, t->private_data->get_size() - offset));

  show_packetizer_info(t->tnum, *t->ptzr_ptr);
}

#endif  // HAVE_FLAC_FORMAT_H

void
kax_reader_c::create_av1_video_packetizer(kax_track_t *t,
                                          track_info_c &nti) {
  if (   (m_writing_app     == "mkvmerge")
      && (m_writing_app_ver != -1)
      && (m_writing_app_ver <= writing_app_ver(28, 0, 0, 0))) {
    // mkvmerge 28.0.0 created invalid av1C CodecPrivate data. Let's rebuild it.
    nti.m_private_data.reset();
  }

  set_track_packetizer(t, new av1_video_packetizer_c(this, nti, t->v_width, t->v_height));
  show_packetizer_info(t->tnum, *t->ptzr_ptr);
  t->handle_packetizer_pixel_dimensions();
  t->handle_packetizer_default_duration();
}

void
kax_reader_c::create_hevc_es_video_packetizer(kax_track_t *t,
                                              track_info_c &nti) {
  set_track_packetizer(t, new hevc_es_video_packetizer_c(this, nti, t->v_width, t->v_height));

  show_packetizer_info(t->tnum, *t->ptzr_ptr);
}

void
kax_reader_c::create_hevc_video_packetizer(kax_track_t *t,
                                           track_info_c &nti) {
  if (t->ms_compat || !nti.m_private_data || !nti.m_private_data->get_size()) {
    create_hevc_es_video_packetizer(t, nti);
    return;
  }

  auto ptzr = new hevc_video_packetizer_c(this, nti, t->default_duration, t->v_width, t->v_height);
  ptzr->set_source_timestamp_resolution(m_tc_scale);

  set_track_packetizer(t, ptzr);
  show_packetizer_info(t->tnum, *t->ptzr_ptr);
}

void
kax_reader_c::create_mp3_audio_packetizer(kax_track_t *t,
                                          track_info_c &nti) {
  set_track_packetizer(t, new mp3_packetizer_c(this, nti, t->a_sfreq, t->a_channels, true));
  show_packetizer_info(t->tnum, *t->ptzr_ptr);
}

void
kax_reader_c::create_opus_audio_packetizer(kax_track_t *t,
                                           track_info_c &nti) {
  set_track_packetizer(t, new opus_packetizer_c(this, nti));
  show_packetizer_info(t->tnum, *t->ptzr_ptr);

  if (!m_opus_experimental_warning_shown && (t->codec_id == std::string{MKV_A_OPUS} + "/EXPERIMENTAL")) {
    mxwarn(fmt::format(FY("'{0}': You're copying an Opus track that was written in experimental mode. "
                          "The resulting track will be written in final mode, but one detail cannot be recovered from a track written in experimental mode: the end trimming. "
                          "This means that a decoder might output a few samples more than originally intended. "
                          "You should re-multiplex from the original Opus file if possible.\n"),
                       m_ti.m_fname));
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
  show_packetizer_info(t->tnum, *t->ptzr_ptr);
}

void
kax_reader_c::create_truehd_audio_packetizer(kax_track_t *t,
                                             track_info_c &nti) {
  nti.m_private_data.reset();
  set_track_packetizer(t, new truehd_packetizer_c(this, nti, t->codec.is(codec_c::type_e::A_TRUEHD) ? mtx::truehd::frame_t::truehd : mtx::truehd::frame_t::mlp, t->a_sfreq, t->a_channels));
  show_packetizer_info(t->tnum, *t->ptzr_ptr);
}

void
kax_reader_c::create_tta_audio_packetizer(kax_track_t *t,
                                          track_info_c &nti) {
  set_track_packetizer(t, new tta_packetizer_c(this, nti, t->a_channels, t->a_bps, t->a_sfreq));
  show_packetizer_info(t->tnum, *t->ptzr_ptr);
}

void
kax_reader_c::create_vorbis_audio_packetizer(kax_track_t *t,
                                             track_info_c &nti) {
  set_track_packetizer(t, new vorbis_packetizer_c(this, nti, t->headers));
  show_packetizer_info(t->tnum, *t->ptzr_ptr);
}

void
kax_reader_c::create_wavpack_audio_packetizer(kax_track_t *t,
                                              track_info_c &nti) {
  nti.m_private_data = t->private_data;

  mtx::wavpack::meta_t meta;
  meta.bits_per_sample = t->a_bps;
  meta.channel_count   = t->a_channels;
  meta.sample_rate     = t->a_sfreq;
  meta.has_correction  = t->max_blockadd_id != 0;

  if (0 < t->default_duration)
    meta.samples_per_block = t->a_sfreq * t->default_duration / 1'000'000'000.0;

  set_track_packetizer(t, new wavpack_packetizer_c(this, nti, meta));

  show_packetizer_info(t->tnum, *t->ptzr_ptr);
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
kax_reader_c::create_dvbsub_subtitle_packetizer(kax_track_t &t,
                                                track_info_c &nti) {
  if (t.private_data->get_size() == 4) {
    // The subtitling type byte is missing. Add it. From ETSI EN 300 468 table 26:
    // 0x10 = DVB subtitles (normal) with no monitor aspect ratio criticality

    t.private_data->resize(5);
    t.private_data->get_buffer()[4] = 0x10;
  }

  set_track_packetizer(&t, new dvbsub_packetizer_c(this, nti, t.private_data));
  show_packetizer_info(t.tnum, *t.ptzr_ptr);
  t.sub_type = 'p';
}

void
kax_reader_c::create_subtitle_packetizer(kax_track_t *t,
                                         track_info_c &nti) {
  if (t->codec.is(codec_c::type_e::S_VOBSUB)) {
    set_track_packetizer(t, new vobsub_packetizer_c(this, nti));
    show_packetizer_info(t->tnum, *t->ptzr_ptr);

    t->sub_type = 'v';

  } else if (t->codec.is(codec_c::type_e::S_DVBSUB))
    create_dvbsub_subtitle_packetizer(*t, nti);

  else if (t->codec.is(codec_c::type_e::S_WEBVTT)) {
    set_track_packetizer(t, new webvtt_packetizer_c(this, nti));
    show_packetizer_info(t->tnum, *t->ptzr_ptr);

    t->sub_type = 't';

  } else if (balg::starts_with(t->codec_id, "S_TEXT") || (t->codec_id == "S_SSA") || (t->codec_id == "S_ASS")) {
    std::string new_codec_id = ((t->codec_id == "S_SSA") || (t->codec_id == "S_ASS")) ? "S_TEXT/"s + std::string(&t->codec_id[2]) : t->codec_id;

    auto recoding_requested = mtx::includes(m_ti.m_sub_charsets, t->tnum) || mtx::includes(m_ti.m_sub_charsets, t->tnum);

    if (codec_c::look_up(new_codec_id).get_type() == codec_c::type_e::S_SSA_ASS)
      set_track_packetizer(t, new ssa_packetizer_c(this, nti, new_codec_id.c_str(), recoding_requested));
    else
      set_track_packetizer(t, new textsubs_packetizer_c(this, nti, new_codec_id.c_str(), recoding_requested));

    show_packetizer_info(t->tnum, *t->ptzr_ptr);

    t->sub_type = 't';

  } else if (t->codec.is(codec_c::type_e::S_KATE)) {
    set_track_packetizer(t, new kate_packetizer_c(this, nti));
    show_packetizer_info(t->tnum, *t->ptzr_ptr);
    t->sub_type = 'k';

  } else if (t->codec.is(codec_c::type_e::S_HDMV_PGS)) {
    set_track_packetizer(t, new hdmv_pgs_packetizer_c(this, nti));
    show_packetizer_info(t->tnum, *t->ptzr_ptr);
    t->sub_type = 'p';

  } else if (t->codec.is(codec_c::type_e::S_HDMV_TEXTST)) {
    set_track_packetizer(t, new hdmv_textst_packetizer_c(this, nti, t->private_data));
    show_packetizer_info(t->tnum, *t->ptzr_ptr);
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
  show_packetizer_info(t->tnum, *t->ptzr_ptr);
}

void
kax_reader_c::create_packetizer(int64_t tid) {
  kax_track_t *t = m_tracks[tid].get();

  if ((-1 != t->ptzr) || !t->ok || !demuxing_requested(t->type, t->tnum, t->effective_language))
    return;

  track_info_c nti(m_ti);
  nti.m_private_data = t->private_data ? t->private_data->clone() : memory_cptr{};
  nti.m_id           = t->tnum; // ID for this track.

  if (!nti.m_language.is_valid())
    nti.m_language   = t->effective_language;
  if (nti.m_track_name == "")
    nti.m_track_name = t->track_name;
  if (t->tags && demuxing_requested('T', t->tnum))
    nti.m_tags       = clone(t->tags);

  if (mtx::hacks::is_engaged(mtx::hacks::FORCE_PASSTHROUGH_PACKETIZER)) {
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
  m_ptzr_to_track_map[ &ptzr(t->ptzr) ] = t;
}

void
kax_reader_c::create_packetizers() {
  m_in->save_pos();

  for (auto &track : m_tracks)
    create_packetizer(track->tnum);

  maybe_set_segment_title(m_ti.m_title);

  m_in->restore_pos();
}

void
kax_reader_c::create_avc_es_video_packetizer(kax_track_t *t,
                                             track_info_c &nti) {
  set_track_packetizer(t, new avc_es_video_packetizer_c(this, nti, t->v_width, t->v_height));

  show_packetizer_info(t->tnum, *t->ptzr_ptr);
}

void
kax_reader_c::create_avc_video_packetizer(kax_track_t *t,
                                          track_info_c &nti) {
  if (!nti.m_private_data || !nti.m_private_data->get_size()) {
    create_avc_es_video_packetizer(t, nti);
    return;
  }

  set_track_packetizer(t, new avc_video_packetizer_c(this, nti, t->default_duration, t->v_width, t->v_height));
  show_packetizer_info(t->tnum, *t->ptzr_ptr);
}

void
kax_reader_c::create_prores_video_packetizer(kax_track_t &t,
                                             track_info_c &nti) {
  set_track_packetizer(&t, new prores_video_packetizer_c{this, nti, t.default_duration, static_cast<int>(t.v_width), static_cast<int>(t.v_height)});
  show_packetizer_info(t.tnum, *t.ptzr_ptr);
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
  show_packetizer_info(t->tnum, *t->ptzr_ptr);

  if (t->private_data && (sizeof(alBITMAPINFOHEADER) < t->private_data->get_size()))
    t->ptzr_ptr->process(std::make_shared<packet_t>(memory_c::borrow(t->private_data->get_buffer() + sizeof(alBITMAPINFOHEADER), t->private_data->get_size() - sizeof(alBITMAPINFOHEADER))));
}

void
kax_reader_c::read_first_frames(kax_track_t *t,
                                unsigned num_wanted) {
  if (t->first_frames_data.size() >= num_wanted)
    return;

  std::map<int64_t, unsigned int> frames_by_track_id;

  try {
    while (true) {
      auto cluster = m_in_file->read_next_cluster();
      if (!cluster)
        return;

      auto ctc = static_cast<kax_cluster_timestamp_c *> (cluster->FindFirstElt(EBML_INFO(kax_cluster_timestamp_c), false));
      if (ctc)
        init_timestamp(*cluster, ctc->GetValue(), m_tc_scale);

      size_t bgidx;
      for (bgidx = 0; bgidx < cluster->ListSize(); bgidx++) {
        if (is_type<libmatroska::KaxSimpleBlock>((*cluster)[bgidx])) {
          libmatroska::KaxSimpleBlock *block_simple = static_cast<libmatroska::KaxSimpleBlock *>((*cluster)[bgidx]);

          block_simple->SetParent(*cluster);
          kax_track_t *block_track = find_track_by_num(block_simple->TrackNum());

          if (!block_track || (0 == block_simple->NumberFrames()))
            continue;

          for (int frame_idx = 0, num_frames = block_simple->NumberFrames(); frame_idx < num_frames; ++frame_idx) {
            frames_by_track_id[ block_simple->TrackNum() ]++;

            if (frames_by_track_id[ block_simple->TrackNum() ] <= block_track->first_frames_data.size())
              continue;

            auto &data_buffer = block_simple->GetBuffer(frame_idx);
            block_track->first_frames_data.push_back(memory_c::borrow(data_buffer.Buffer(), data_buffer.Size()));
            block_track->content_decoder.reverse(block_track->first_frames_data.back(), CONTENT_ENCODING_SCOPE_BLOCK);
            block_track->first_frames_data.back()->take_ownership();
          }

        } else if (is_type<libmatroska::KaxBlockGroup>((*cluster)[bgidx])) {
          auto block_group = static_cast<libmatroska::KaxBlockGroup *>((*cluster)[bgidx]);
          auto block       = static_cast<libmatroska::KaxBlock *>(block_group->FindFirstElt(EBML_INFO(libmatroska::KaxBlock), false));

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

            auto &data_buffer = block->GetBuffer(frame_idx);
            block_track->first_frames_data.push_back(memory_cptr(memory_c::borrow(data_buffer.Buffer(), data_buffer.Size())));
            block_track->content_decoder.reverse(block_track->first_frames_data.back(), CONTENT_ENCODING_SCOPE_BLOCK);
            block_track->first_frames_data.back()->take_ownership();
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
                   [[maybe_unused]] bool force) {
  if (m_tracks.empty() || (FILE_STATUS_DONE == m_file_status))
    return FILE_STATUS_DONE;

  auto num_queued_bytes = get_queued_bytes();

  if (20 * 1024 * 1024 < num_queued_bytes) {
    auto requested_ptzr_track = m_ptzr_to_track_map[requested_ptzr];
    if (   !requested_ptzr_track
        || (('a' != requested_ptzr_track->type) && ('v' != requested_ptzr_track->type))
        || (128 * 1024 * 1024 < num_queued_bytes))
      return FILE_STATUS_HOLDING;
  }

  try {
    auto cluster = m_in_file->read_next_cluster();
    if (!cluster)
      return finish_file();

    auto cluster_ts = find_child_value<kax_cluster_timestamp_c>(*cluster);
    init_timestamp(*cluster, cluster_ts, m_tc_scale);

    size_t bgidx;
    for (bgidx = 0; bgidx < cluster->ListSize(); bgidx++) {
      libebml::EbmlElement *element = (*cluster)[bgidx];

      if (is_type<libmatroska::KaxSimpleBlock>(element))
        process_simple_block(cluster.get(), static_cast<libmatroska::KaxSimpleBlock *>(element));

      else if (is_type<libmatroska::KaxBlockGroup>(element))
        process_block_group(cluster.get(), static_cast<libmatroska::KaxBlockGroup *>(element));
    }

  } catch (...) {
    mxwarn(fmt::format("{0} {1} {2}\n",
                       fmt::format(FY("{0}: an unknown exception occurred."), "kax_reader_c::read()"),
                       Y("This usually indicates a damaged file structure."), Y("The file will not be processed further.")));
    return finish_file();
  }

  return FILE_STATUS_MOREDATA;
}

file_status_e
kax_reader_c::finish_file() {
  flush_packetizers();

  m_in->setFilePointer(0, libebml::seek_end);
  m_file_status = FILE_STATUS_DONE;

  return FILE_STATUS_DONE;
}

void
kax_reader_c::process_simple_block(libmatroska::KaxCluster *cluster,
                                   libmatroska::KaxSimpleBlock *block_simple) {
  int64_t block_duration = -1;
  int64_t block_bref     = VFT_IFRAME;
  int64_t block_fref     = VFT_NOBFRAME;

  block_simple->SetParent(*cluster);
  auto block_track     = find_track_by_num(block_simple->TrackNum());
  auto block_timestamp = mtx::math::to_signed(get_global_timestamp(*block_simple)) - m_global_timestamp_offset;

  if (!block_track) {
    if (!m_known_bad_track_numbers[block_simple->TrackNum()])
      mxwarn_fn(m_ti.m_fname,
                fmt::format(FY("A block was found at timestamp {0} for track number {1}. However, no headers were found for that track number. "
                               "The block will be skipped.\n"), mtx::string::format_timestamp(block_timestamp), block_simple->TrackNum()));
    return;
  }

  if (0 < block_track->default_duration)
    block_duration = block_track->default_duration;
  int64_t frame_duration = (block_duration == -1) ? 0 : block_duration;

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

  if ((-1 != block_track->ptzr) && block_track->passthrough) {
    // The handling for passthrough is a bit different. We don't have
    // any special cases, e.g. 0 terminating a string for the subs
    // and stuff. Just pass everything through as it is.
    size_t i;
    for (i = 0; block_simple->NumberFrames() > i; ++i) {
      auto &data_buffer = block_simple->GetBuffer(i);
      auto data         = memory_c::borrow(data_buffer.Buffer(), data_buffer.Size());
      block_track->content_decoder.reverse(data, CONTENT_ENCODING_SCOPE_BLOCK);

      packet_cptr packet(new packet_t(data, m_last_timestamp + i * frame_duration, block_duration, block_bref, block_fref));
      packet->key_flag         = key_flag;
      packet->discardable_flag = discardable_flag;

      ptzr(block_track->ptzr).process(packet);
    }

  } else if (-1 != block_track->ptzr) {
    size_t i;
    for (i = 0; i < block_simple->NumberFrames(); i++) {
      auto &data_buffer = block_simple->GetBuffer(i);
      auto data         = memory_c::borrow(data_buffer.Buffer(), data_buffer.Size());
      block_track->content_decoder.reverse(data, CONTENT_ENCODING_SCOPE_BLOCK);

      auto packet              = std::make_shared<packet_t>(data, m_last_timestamp + i * frame_duration, block_duration, block_bref, block_fref);
      packet->key_flag         = key_flag;
      packet->discardable_flag = discardable_flag;

      ptzr(block_track->ptzr).process(packet);
    }
  }

  block_track->previous_timestamp  = m_last_timestamp;
  block_track->units_processed    += block_simple->NumberFrames();
}

void
kax_reader_c::process_block_group_common(libmatroska::KaxBlockGroup *block_group,
                                         packet_t *packet,
                                         kax_track_t &block_track) {
  auto codec_state     = find_child<libmatroska::KaxCodecState>(block_group);
  auto discard_padding = find_child<libmatroska::KaxDiscardPadding>(block_group);
  auto blockadd        = find_child<libmatroska::KaxBlockAdditions>(block_group);

  if (codec_state)
    packet->codec_state = memory_c::clone(codec_state->GetBuffer(), codec_state->GetSize());

  if (discard_padding)
    packet->discard_padding = timestamp_c::ns(discard_padding->GetValue());

  if (!blockadd)
    return;

  for (auto &child : *blockadd) {
    if (!(is_type<libmatroska::KaxBlockMore>(child)))
      continue;

    auto blockmore  = static_cast<libmatroska::KaxBlockMore *>(child);
    auto k_blockadd = find_child<libmatroska::KaxBlockAdditional>(*blockmore);

    if (!k_blockadd)
      continue;

    block_add_t add{ memory_c::borrow(k_blockadd->GetBuffer(), k_blockadd->GetSize()) };

    block_track.content_decoder.reverse(add.data, CONTENT_ENCODING_SCOPE_BLOCK);

    auto k_blockadd_id = find_child<libmatroska::KaxBlockAddID>(*blockmore);
    add.id             = k_blockadd_id ? k_blockadd_id->GetValue() : 1;

    packet->data_adds.push_back(add);

    if (m_is_webm)
      block_track.register_use_of_webm_block_addition_id(add.id.value());
  }
}

void
kax_reader_c::process_block_group(libmatroska::KaxCluster *cluster,
                                  libmatroska::KaxBlockGroup *block_group) {
  auto block = find_child<libmatroska::KaxBlock>(block_group);
  if (!block)
    return;

  block->SetParent(*cluster);
  auto block_track     = find_track_by_num(block->TrackNum());
  auto block_timestamp = mtx::math::to_signed(get_global_timestamp(*block)) - m_global_timestamp_offset;

  if (!block_track) {
    if (!m_known_bad_track_numbers[block->TrackNum()])
      mxwarn_fn(m_ti.m_fname,
                fmt::format(FY("A block was found at timestamp {0} for track number {1}. However, no headers were found for that track number. "
                               "The block will be skipped.\n"), mtx::string::format_timestamp(block_timestamp), block->TrackNum()));
    return;
  }

  auto duration       = find_child<libmatroska::KaxBlockDuration>(block_group);
  auto block_duration = duration                      ? static_cast<int64_t>(duration->GetValue() * m_tc_scale / block->NumberFrames())
                      : block_track->default_duration ? block_track->default_duration
                      :                                 int64_t{-1};
  auto frame_duration = -1 == block_duration          ? int64_t{0} : block_duration;
  m_last_timestamp    = block_timestamp;

  if (0 < block->NumberFrames())
    m_in_file->set_last_timestamp(m_last_timestamp + (block->NumberFrames() - 1) * frame_duration);

  if (-1 == block_track->ptzr)
    return;

  auto block_bref = int64_t{VFT_IFRAME};
  auto block_fref = int64_t{VFT_NOBFRAME};
  bool bref_found = false;
  bool fref_found = false;
  auto ref_block  = find_child<libmatroska::KaxReferenceBlock>(block_group);

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
      auto data         = memory_c::borrow(data_buffer.Buffer(), data_buffer.Size());
      block_track->content_decoder.reverse(data, CONTENT_ENCODING_SCOPE_BLOCK);

      auto packet                = std::make_shared<packet_t>(data, m_last_timestamp + i * frame_duration, block_duration, block_bref, block_fref);
      packet->duration_mandatory = duration;

      process_block_group_common(block_group, packet.get(), *block_track);

      ptzr(block_track->ptzr).process(packet);
    }

    return;
  }

  if (bref_found)
    block_bref += m_last_timestamp;
  if (fref_found)
    block_fref += m_last_timestamp;

  for (auto block_idx = 0u, num_frames = block->NumberFrames(); block_idx < num_frames; ++block_idx) {
    auto &data_buffer = block->GetBuffer(block_idx);
    auto data         = memory_c::borrow(data_buffer.Buffer(), data_buffer.Size());
    block_track->content_decoder.reverse(data, CONTENT_ENCODING_SCOPE_BLOCK);

    auto packet = std::make_shared<packet_t>(data, m_last_timestamp + block_idx * frame_duration, block_duration, block_bref, block_fref);

    if (duration && !duration->GetValue())
      packet->duration_mandatory = true;

    process_block_group_common(block_group, packet.get(), *block_track);

    ptzr(block_track->ptzr).process(packet);
  }

  block_track->previous_timestamp  = m_last_timestamp;
  block_track->units_processed    += block->NumberFrames();
}

void
kax_reader_c::set_headers() {
  generic_reader_c::set_headers();

  for (auto &track : m_tracks)
    if ((-1 != track->ptzr) && track->passthrough)
      ptzr(track->ptzr).get_track_entry()->EnableLacing(track->lacing_flag);
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
  auto done             = false;

  for (auto &track : m_tracks)
    tracks_by_number[track->track_number] = track;

  while (!done) {
    try {
      auto cluster = m_in_file->read_next_cluster();
      if (!cluster)
        break;

      auto cluster_ts = find_child_value<kax_cluster_timestamp_c>(*cluster);
      init_timestamp(*cluster, cluster_ts, m_tc_scale);

      for (auto const &element : *cluster) {
        uint64_t track_number{};

        if (is_type<libmatroska::KaxSimpleBlock>(element)) {
          auto block = static_cast<libmatroska::KaxSimpleBlock *>(element);
          block->SetParent(*cluster);

          last_timestamp = timestamp_c::ns(mtx::math::to_signed(get_global_timestamp(*block)));
          track_number   = block->TrackNum();

        } else if (is_type<libmatroska::KaxBlockGroup>(element)) {
          auto block = find_child<libmatroska::KaxBlock>(static_cast<libmatroska::KaxBlockGroup *>(element));
          if (!block)
            continue;

          block->SetParent(*cluster);

          last_timestamp = timestamp_c::ns(mtx::math::to_signed(get_global_timestamp(*block)));
          track_number   = block->TrackNum();

        } else
          continue;

        if (!first_timestamp.valid())
          first_timestamp = last_timestamp;

        if ((last_timestamp - first_timestamp) >= probe_time_limit) {
          done = true;
          break;
        }

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
        if (tracks_by_number.empty()) {
          done = true;
          break;
        }
      }

    } catch (...) {
      break;
    }
  }

  if (!m_debug_minimum_timestamp)
    return;

  auto track_numbers = mtx::keys(m_minimum_timestamps_by_track_number);
  std::sort(track_numbers.begin(), track_numbers.end());

  mxdebug("Minimum timestamps by track number:\n");

  for (auto const &track_number : track_numbers)
    mxdebug(fmt::format("  {0}: {1}\n", track_number, m_minimum_timestamps_by_track_number[track_number]));
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

  auto use_value = global_minimum_timestamp.valid()
                && ((global_minimum_timestamp < timestamp_c::ns(0)) || m_appending);

  if (use_value)
    m_global_timestamp_offset = global_minimum_timestamp.to_ns();

  mxdebug_if(m_debug_minimum_timestamp, fmt::format("Global minimum timestamp: {0}; {1}using it\n", global_minimum_timestamp, use_value ? "" : "not "));
}

void
kax_reader_c::identify() {
  auto remove_track_statistics_tags = !mtx::hacks::is_engaged(mtx::hacks::KEEP_TRACK_STATISTICS_TAGS);

  auto info = mtx::id::info_c{};

  info.set(mtx::id::muxing_application,  m_muxing_app);
  info.set(mtx::id::writing_application, m_raw_writing_app);
  info.add(mtx::id::title,               m_ti.m_title);
  info.add(mtx::id::duration,            m_segment_duration);
  info.add(mtx::id::timestamp_scale,     m_tc_scale);

  auto add_uid_info = [&info](memory_cptr const &uid, std::string const &property) {
    if (uid)
      info.add(property, mtx::string::to_hex(uid, true));
  };
  add_uid_info(m_segment_uid,          mtx::id::segment_uid);
  add_uid_info(m_next_segment_uid,     mtx::id::next_segment_uid);
  add_uid_info(m_previous_segment_uid, mtx::id::previous_segment_uid);

  if (m_muxing_date_epoch) {
    auto timestamp = QDateTime::fromSecsSinceEpoch(m_muxing_date_epoch.value(), QTimeZone::utc());
    info.add(mtx::id::date_utc,   mtx::date_time::format_iso_8601(timestamp));
    info.add(mtx::id::date_local, mtx::date_time::format_iso_8601(timestamp.toLocalTime()));
  }

  id_result_container(info.get());

  for (auto &track : m_tracks) {
    if (!track->ok)
      continue;

    info = mtx::id::info_c{};

    info.add(mtx::id::number,                 track->track_number);
    info.add(mtx::id::uid,                    track->track_uid);
    info.set(mtx::id::num_index_entries,      track->num_cue_points);
    info.add(mtx::id::codec_id,               track->codec_id);
    info.set(mtx::id::codec_private_length,   track->private_data ? track->private_data->get_size() : 0u);
    info.add(mtx::id::codec_private_data,     track->private_data);
    info.add(mtx::id::codec_delay,            track->codec_delay.to_ns(0));
    info.add(mtx::id::codec_name,             track->codec_name);
    info.add(mtx::id::language,               track->language.get_iso639_alpha_3_code());
    info.add(mtx::id::language_ietf,          track->language_ietf.format());
    info.add(mtx::id::track_name,             track->track_name);
    info.add(mtx::id::stereo_mode,            static_cast<int>(track->v_stereo_mode), static_cast<int>(stereo_mode_c::unspecified));
    info.add(mtx::id::alpha_mode,             track->v_alpha_mode);
    info.add(mtx::id::default_duration,       track->default_duration);
    info.set(mtx::id::default_track,          track->default_track ? true : false);
    info.set(mtx::id::forced_track,           track->forced_track  ? true : false);
    info.set(mtx::id::enabled_track,          track->enabled_track ? true : false);
    info.add(mtx::id::flag_hearing_impaired,  track->hearing_impaired_flag);
    info.add(mtx::id::flag_visual_impaired,   track->visual_impaired_flag);
    info.add(mtx::id::flag_text_descriptions, track->text_descriptions_flag);
    info.add(mtx::id::flag_original,          track->original_flag);
    info.add(mtx::id::flag_commentary,        track->commentary_flag);
    info.add(mtx::id::display_unit,           track->v_dunit);

    info.add_joined(mtx::id::pixel_dimensions,   "x"s, track->v_width, track->v_height);
    info.add_joined(mtx::id::display_dimensions, "x"s, track->v_dwidth, track->v_dheight);
    info.add_joined(mtx::id::cropping,           ","s, track->v_pcleft, track->v_pctop, track->v_pcright, track->v_pcbottom);

    if (track->codec.is(codec_c::type_e::V_MPEG4_P10))
      info.add(mtx::id::packetizer, track->ms_compat ? mtx::id::mpeg4_p10_es_video : mtx::id::mpeg4_p10_video);
    else if (track->codec.is(codec_c::type_e::V_MPEGH_P2))
      info.add(mtx::id::packetizer, track->ms_compat ? mtx::id::mpegh_p2_es_video  : mtx::id::mpegh_p2_video);

    if ('a' == track->type) {
      info.add(mtx::id::audio_sampling_frequency, static_cast<int64_t>(track->a_sfreq));
      info.add(mtx::id::audio_channels,           track->a_channels);
      info.add(mtx::id::audio_bits_per_sample,    track->a_bps);
      info.add(mtx::id::audio_emphasis,           track->a_emphasis);

    } else if ('s' == track->type) {
      if (track->codec.is(codec_c::type_e::S_SRT) || track->codec.is(codec_c::type_e::S_SSA_ASS) || track->codec.is(codec_c::type_e::S_KATE)) {
        info.add(mtx::id::text_subtitles, true);
        info.add(mtx::id::encoding, "UTF-8");
      }

    } else if ('v' == track->type) {
      auto maybe_set_int = [&info](std::string const &key, std::optional<uint64_t> const &v1, std::optional<uint64_t> const &v2) {
        if (v1 || v2)
          info.set(key, fmt::format("{0},{1}", v1.value_or(0), v2.value_or(0)));
      };

      auto maybe_set_float = [&info](std::string const &key, std::optional<double> const &v1, std::optional<double> const &v2) {
        if (v1 || v2)
          info.set(key, fmt::format("{0},{1}", mtx::string::normalize_fmt_double_output(v1.value_or(0)), mtx::string::normalize_fmt_double_output(v2.value_or(0))));
      };

      info.add(mtx::id::color_bits_per_channel,         track->v_bits_per_channel);
      info.add(mtx::id::color_matrix_coefficients,      track->v_color_matrix);
      info.add(mtx::id::color_primaries,                track->v_color_primaries);
      info.add(mtx::id::color_range,                    track->v_color_range);
      info.add(mtx::id::color_transfer_characteristics, track->v_transfer_character);
      info.add(mtx::id::max_content_light,              track->v_max_cll);
      info.add(mtx::id::max_frame_light,                track->v_max_fall);
      info.add(mtx::id::max_luminance,                  track->v_max_luminance);
      info.add(mtx::id::min_luminance,                  track->v_min_luminance);
      info.add(mtx::id::projection_pose_pitch,          track->v_projection_pose_pitch);
      info.add(mtx::id::projection_pose_roll,           track->v_projection_pose_roll);
      info.add(mtx::id::projection_pose_yaw,            track->v_projection_pose_yaw);
      info.add(mtx::id::projection_private,             track->v_projection_private);
      info.add(mtx::id::projection_type,                track->v_projection_type);

      maybe_set_int(  mtx::id::cb_subsample,            track->v_cb_subsample.hori,            track->v_cb_subsample.vert);
      maybe_set_int(  mtx::id::chroma_siting,           track->v_chroma_siting.hori,           track->v_chroma_siting.vert);
      maybe_set_int(  mtx::id::chroma_subsample,        track->v_chroma_subsample.hori,        track->v_chroma_subsample.vert);
      maybe_set_float(mtx::id::white_color_coordinates, track->v_white_color_coordinates.hori, track->v_white_color_coordinates.vert);

      if (   track->v_chroma_coordinates.red_x   || track->v_chroma_coordinates.red_y
          || track->v_chroma_coordinates.green_x || track->v_chroma_coordinates.green_y
          || track->v_chroma_coordinates.blue_x  || track->v_chroma_coordinates.blue_y)
        info.set(mtx::id::chromaticity_coordinates, fmt::format("{0},{1},{2},{3},{4},{5}",
                                                                mtx::string::normalize_fmt_double_output(track->v_chroma_coordinates.red_x.value_or(0)),   mtx::string::normalize_fmt_double_output(track->v_chroma_coordinates.red_y.value_or(0)),
                                                                mtx::string::normalize_fmt_double_output(track->v_chroma_coordinates.green_x.value_or(0)), mtx::string::normalize_fmt_double_output(track->v_chroma_coordinates.green_y.value_or(0)),
                                                                mtx::string::normalize_fmt_double_output(track->v_chroma_coordinates.blue_x.value_or(0)),  mtx::string::normalize_fmt_double_output(track->v_chroma_coordinates.blue_y.value_or(0))));
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
        codec_info = fmt::format(FY("unknown, format tag 0x{0:04x}"), track->a_formattag);

    } else
      codec_info = track->codec_id;

    if (track->tags)
      add_track_tags_to_identification(*track->tags, info);

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

    if (remove_track_statistics_tags)
      track->discard_track_statistics_tags();
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

  if (m_chapters)
    add_available_track_id(track_info_c::chapter_track_id);
}

void
kax_reader_c::set_track_packetizer(kax_track_t *t,
                                   generic_packetizer_c *packetizer) {
  t->ptzr     = add_packetizer(packetizer);
  t->ptzr_ptr = packetizer;
}

void
kax_reader_c::handle_vorbis_comments_cover_art(mtx::tags::converted_vorbis_comments_t const &converted) {
  for (auto const &picture : converted.m_pictures) {
    picture->ui_id        = ++m_attachment_id;
    picture->source_file  = m_ti.m_fname;
    auto attach_mode      = attachment_requested(picture->ui_id);
    picture->to_all_files = ATTACH_MODE_TO_ALL_FILES == attach_mode;

    if (ATTACH_MODE_SKIP != attach_mode)
      add_attachment(picture);
  }
}

void
kax_reader_c::handle_vorbis_comments_tags(mtx::tags::converted_vorbis_comments_t const &converted,
                                          kax_track_t &t) {
  if (!converted.m_track_tags && !converted.m_album_tags)
    return;

  t.tags = mtx::tags::merge(t.tags, mtx::tags::merge(converted.m_track_tags, converted.m_album_tags));
}

void
kax_reader_c::handle_vorbis_comments(kax_track_t &t) {
  auto comments = mtx::tags::parse_vorbis_comments_from_packet(*t.headers[1]);
  if (!comments.valid())
    return;

  auto converted = mtx::tags::from_vorbis_comments(comments);

  handle_vorbis_comments_cover_art(converted);
  handle_vorbis_comments_tags(converted, t);

  comments.m_comments.clear();
  t.headers[1] = mtx::tags::assemble_vorbis_comments_into_packet(comments);
}
