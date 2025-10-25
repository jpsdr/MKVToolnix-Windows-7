/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   AVI demultiplexer module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <algorithm>
#include <cmath>

#include "avilib.h"
#include "common/aac.h"
#include "common/avc/util.h"
#include "common/codec.h"
#include "common/endian.h"
#include "common/error.h"
#include "common/hacks.h"
#include "common/mm_mem_io.h"
#include "common/mm_proxy_io.h"
#include "common/mm_text_io.h"
#include "common/mpeg1_2.h"
#include "common/mpeg4_p2.h"
#include "common/strings/formatting.h"
#include "common/id_info.h"
#include "input/r_avi.h"
#include "input/subtitles.h"
#include "merge/input_x.h"
#include "merge/file_status.h"
#include "output/p_aac.h"
#include "output/p_ac3.h"
#include "output/p_avc_es.h"
#include "output/p_dts.h"
#include "output/p_mp3.h"
#include "output/p_mpeg1_2.h"
#include "output/p_mpeg4_p2.h"
#include "output/p_pcm.h"
#include "output/p_video_for_windows.h"
#include "output/p_vorbis.h"
#include "output/p_vpx.h"

extern "C" {
extern long AVI_errno;
}

constexpr auto AVI_MAX_AUDIO_CHUNK_SIZE = (10 * 1024 * 1024);

constexpr auto GAB2_TAG                 = mtx::calc_fourcc('G', 'A', 'B', '2');
constexpr auto GAB2_ID_SUBTITLES        = 0x0004;

bool
avi_reader_c::probe_file() {
  auto &in  = *m_in;

  std::string data;
  if (in.read(data, 12) != 12)
    return false;

  balg::to_lower(data);
  if ((data.substr(0, 4) != "riff") || (data.substr(8, 4) != "avi "))
    return false;

  auto avi       = AVI_open_input_file(&in, 1);
  auto const err = AVI_errno;

  if (avi)
    AVI_close(avi);

  else if (err == AVI_ERR_UNSUPPORTED_DV_TYPE1)
    id_result_container_unsupported(in.get_file_name(), mtx::file_type_t::get_name(mtx::file_type_e::avi_dv_1));

  return true;
}

void
avi_reader_c::read_headers() {
  show_demuxer_info();

  if (!(m_avi = AVI_open_input_file(m_in.get(), 1)))
    throw mtx::input::invalid_format_x();

  auto frame_rate    = AVI_frame_rate(m_avi);
  m_default_duration = mtx::rational(1'000'000'000.0, frame_rate ? frame_rate : 25);
  m_max_video_frames = AVI_video_frames(m_avi);
  m_video_width      = std::abs(AVI_video_width(m_avi));
  m_video_height     = std::abs(AVI_video_height(m_avi));

  handle_video_aspect_ratio();

  verify_video_track();
  parse_subtitle_chunks();

  if (debugging_c::requested("avi_dump_video_index"))
    debug_dump_video_index();
}

avi_reader_c::~avi_reader_c() {
  if (m_avi)
    AVI_close(m_avi);

  mxdebug_if(m_debug, fmt::format("avi_reader_c: Dropped video frames: {0}\n", m_dropped_video_frames));
}

void
avi_reader_c::verify_video_track() {
  auto size        = get_uint32_le(&m_avi->bitmap_info_header->bi_size);
  m_video_track_ok = (sizeof(alBITMAPINFOHEADER) <= size) && (0 != m_video_width) && (0 != m_video_height);
}

void
avi_reader_c::parse_subtitle_chunks() {
  int i;
  for (i = 0; AVI_text_tracks(m_avi) > i; ++i) {
    AVI_set_text_track(m_avi, i);

    if (AVI_text_chunks(m_avi) == 0)
      continue;

    int chunk_size = AVI_read_text_chunk(m_avi, nullptr);
    if (0 >= chunk_size)
      continue;

    memory_cptr chunk(memory_c::alloc(chunk_size));
    chunk_size = AVI_read_text_chunk(m_avi, reinterpret_cast<char *>(chunk->get_buffer()));

    if (0 >= chunk_size)
      continue;

    avi_subs_demuxer_t demuxer;

    try {
      mm_mem_io_c io(chunk->get_buffer(), chunk_size);
      uint32_t tag = io.read_uint32_be();

      if (GAB2_TAG != tag)
        continue;

      io.skip(1);

      while (!io.eof() && (io.getFilePointer() < static_cast<size_t>(chunk_size))) {
        uint16_t id = io.read_uint16_le();
        int len     = io.read_uint32_le();

        if (GAB2_ID_SUBTITLES == id) {
          demuxer.m_subtitles = memory_c::alloc(len);
          len                 = io.read(demuxer.m_subtitles->get_buffer(), len);
          demuxer.m_subtitles->set_size(len);

        } else
          io.skip(len);
      }

      if (0 != demuxer.m_subtitles->get_size()) {
        mm_text_io_c text_io(std::make_shared<mm_mem_io_c>(demuxer.m_subtitles->get_buffer(), demuxer.m_subtitles->get_size()));
        demuxer.m_type
          = srt_parser_c::probe(text_io) ? avi_subs_demuxer_t::TYPE_SRT
          : ssa_parser_c::probe(text_io) ? avi_subs_demuxer_t::TYPE_SSA
          :                                avi_subs_demuxer_t::TYPE_UNKNOWN;
        demuxer.m_encoding = text_io.get_encoding();

        if (avi_subs_demuxer_t::TYPE_UNKNOWN != demuxer.m_type)
          m_subtitle_demuxers.push_back(demuxer);
      }

    } catch (...) {
    }
  }
}

void
avi_reader_c::create_packetizer(int64_t tid) {
  m_ti.m_private_data.reset();

  if ((0 == tid) && demuxing_requested('v', 0) && (-1 == m_vptzr) && m_video_track_ok)
    create_video_packetizer();

  else if ((0 != tid) && (tid <= AVI_audio_tracks(m_avi)) && demuxing_requested('a', tid))
    add_audio_demuxer(tid - 1);
}

void
avi_reader_c::create_video_packetizer() {
  size_t i;

  mxdebug_if(m_debug, fmt::format("'{0}' track {1}: frame sizes:\n", m_ti.m_fname, 0));

  for (i = 0; i < m_max_video_frames; i++) {
    m_bytes_to_process += AVI_frame_size(m_avi, i);
    mxdebug_if(m_debug, fmt::format("  {0}: {1}\n", i, AVI_frame_size(m_avi, i)));
  }

  if (m_avi->bitmap_info_header) {
    m_ti.m_private_data = memory_c::clone(m_avi->bitmap_info_header, sizeof(alBITMAPINFOHEADER) + m_avi->extradata_size);

    mxdebug_if(m_debug, fmt::format("track extra data size: {0}\n", m_ti.m_private_data->get_size() - sizeof(alBITMAPINFOHEADER)));

    if (sizeof(alBITMAPINFOHEADER) < m_ti.m_private_data->get_size())
      mxdebug_if(m_debug, fmt::format("  {0}\n", mtx::string::to_hex(m_ti.m_private_data->get_buffer() + sizeof(alBITMAPINFOHEADER), m_ti.m_private_data->get_size() - sizeof(alBITMAPINFOHEADER))));
  }

  const char *codec = AVI_video_compressor(m_avi);
  if (mtx::mpeg4_p2::is_v3_fourcc(codec))
    m_divx_type = DIVX_TYPE_V3;
  else if (mtx::mpeg4_p2::is_fourcc(codec))
    m_divx_type = DIVX_TYPE_MPEG4;

  if (mtx::includes(m_ti.m_default_durations, 0))
    m_default_duration = mtx::rational(m_ti.m_default_durations[0].first, 1);

  else if (mtx::includes(m_ti.m_default_durations, -1))
    m_default_duration = mtx::rational(m_ti.m_default_durations[-1].first, 1);

  m_ti.m_id = 0;                 // ID for the video track.
  if (DIVX_TYPE_MPEG4 == m_divx_type)
    create_mpeg4_p2_packetizer();

  else if (mtx::avc::is_avc_fourcc(codec) && !mtx::hacks::is_engaged(mtx::hacks::ALLOW_AVC_IN_VFW_MODE))
    create_mpeg4_p10_packetizer();

  else if (mtx::mpeg1_2::is_fourcc(get_uint32_le(codec)))
    create_mpeg1_2_packetizer();

  else if (mtx::calc_fourcc('V', 'P', '8', '0') == get_uint32_be(codec))
    create_vp8_packetizer();

  else
    create_standard_video_packetizer();

  if (m_video_display_width)
    ptzr(m_vptzr).set_video_display_dimensions(m_video_display_width, m_video_display_height, generic_packetizer_c::ddu_pixels, option_source_e::container);
}

void
avi_reader_c::handle_video_aspect_ratio() {
  if (!m_avi->video_properties_valid) {
    mxdebug_if(m_debug_aspect_ratio, fmt::format("handle_video_aspect_ratio: video properties header not present\n"));
    return;
  }

  uint64_t x = get_uint16_le(&m_avi->video_properties.frame_aspect_ratio_x),
           y = get_uint16_le(&m_avi->video_properties.frame_aspect_ratio_y);

  if (!x || !y) {
    mxdebug_if(m_debug_aspect_ratio, fmt::format("handle_video_aspect_ratio: invalid aspect ratio {0}:{1}, ignoring\n", x, y));
    return;
  }

  if (!m_video_width || !m_video_height) {
    mxdebug_if(m_debug_aspect_ratio, fmt::format("handle_video_aspect_ratio: invalid video dimensions {0}x{1}, ignoring\n", m_video_width, m_video_height));
    return;
  }

  auto aspect_ratio = mtx_mp_rational_t{x, y};

  if (aspect_ratio >= mtx_mp_rational_t{m_video_width, m_video_height}) {
    m_video_display_width  = mtx::to_int_rounded(aspect_ratio * m_video_height);
    m_video_display_height = m_video_height;

  } else {
    m_video_display_width  = m_video_width;
    m_video_display_height = mtx::to_int_rounded(mtx_mp_rational_t{y, x} * m_video_width);
  }

  mxdebug_if(m_debug_aspect_ratio, fmt::format("handle_video_aspect_ratio: frame aspect ratio {0}:{1} pixel dimensions {2}x{3} display dimensions {4}x{5}\n",
                                               x, y, m_video_width, m_video_height, m_video_display_width, m_video_display_height));
}

void
avi_reader_c::create_mpeg1_2_packetizer() {
  std::shared_ptr<M2VParser> m2v_parser(new M2VParser);

  m2v_parser->SetProbeMode();
  if (m_ti.m_private_data && (m_ti.m_private_data->get_size() < sizeof(alBITMAPINFOHEADER)))
    m2v_parser->WriteData(m_ti.m_private_data->get_buffer() + sizeof(alBITMAPINFOHEADER), m_ti.m_private_data->get_size() - sizeof(alBITMAPINFOHEADER));

  unsigned int frame_number = 0;
  unsigned int state        = m2v_parser->GetState();
  while ((frame_number < std::min(m_max_video_frames, 100u)) && (MPV_PARSER_STATE_FRAME != state)) {
    ++frame_number;

    int size = AVI_frame_size(m_avi, frame_number - 1);
    if (0 == size)
      continue;

    AVI_set_video_position(m_avi, frame_number - 1);

    memory_cptr buffer = memory_c::alloc(size);
    int key      = 0;
    int num_read = AVI_read_frame(m_avi, reinterpret_cast<char *>(buffer->get_buffer()), &key);

    if (0 >= num_read)
      continue;

    m2v_parser->WriteData(buffer->get_buffer(), num_read);

    state = m2v_parser->GetState();
  }

  AVI_set_video_position(m_avi, 0);

  if (MPV_PARSER_STATE_FRAME != state)
    mxerror_tid(m_ti.m_fname, 0, Y("Could not extract the sequence header from this MPEG-1/2 track.\n"));

  MPEG2SequenceHeader seq_hdr = m2v_parser->GetSequenceHeader();
  std::shared_ptr<MPEGFrame> frame(m2v_parser->ReadFrame());
  if (!frame)
    mxerror_tid(m_ti.m_fname, 0, Y("Could not extract the sequence header from this MPEG-1/2 track.\n"));

  int display_width      = ((0 >= seq_hdr.aspectRatio) || (1 == seq_hdr.aspectRatio)) ? seq_hdr.width : static_cast<int>(seq_hdr.height * seq_hdr.aspectRatio);

  m_vptzr                = add_packetizer(new mpeg1_2_video_packetizer_c(this, m_ti, m2v_parser->GetMPEGVersion(), static_cast<int64_t>(1'000'000'000.0 / seq_hdr.frameRate),
                                                                         seq_hdr.width, seq_hdr.height, display_width, seq_hdr.height, false));

  show_packetizer_info(0, ptzr(m_vptzr));
}

void
avi_reader_c::create_mpeg4_p2_packetizer() {
  m_vptzr = add_packetizer(new mpeg4_p2_video_packetizer_c(this, m_ti, mtx::to_int_rounded(m_default_duration), m_video_width, m_video_height, false));

  show_packetizer_info(0, ptzr(m_vptzr));
}

void
avi_reader_c::create_mpeg4_p10_packetizer() {
  try {
    auto ptzr = new avc_es_video_packetizer_c(this, m_ti, m_video_width, m_video_height);
    m_vptzr   = add_packetizer(ptzr);

    if (0 != m_default_duration)
      ptzr->set_container_default_field_duration(mtx::to_int_rounded(m_default_duration / 2));

    if (0 < m_avi->extradata_size) {
      auto avc_extra_nalus = mtx::avc::avcc_to_nalus(reinterpret_cast<uint8_t *>(m_avi->bitmap_info_header + 1), m_avi->extradata_size);
      if (avc_extra_nalus)
        ptzr->add_extra_data(avc_extra_nalus);
    }

    set_avc_nal_size_size(ptzr);

    show_packetizer_info(0, *ptzr);

  } catch (...) {
    mxerror_tid(m_ti.m_fname, 0, Y("Could not extract the decoder specific config data (AVCC) from this AVC/H.264 track.\n"));
  }
}

void
avi_reader_c::create_vp8_packetizer() {
  m_ti.m_private_data.reset();
  m_vptzr = add_packetizer(new vpx_video_packetizer_c(this, m_ti, codec_c::type_e::V_VP8));

  ptzr(m_vptzr).set_track_default_duration(mtx::to_int_rounded(m_default_duration));
  ptzr(m_vptzr).set_video_pixel_dimensions(m_video_width, m_video_height);

  show_packetizer_info(0, ptzr(m_vptzr));
}

void
avi_reader_c::create_standard_video_packetizer() {
  m_vptzr = add_packetizer(new video_for_windows_packetizer_c(this, m_ti, mtx::to_int_rounded(m_default_duration), m_video_width, m_video_height));

  show_packetizer_info(0, ptzr(m_vptzr));
}

void
avi_reader_c::create_packetizers() {
  int i;

  create_packetizer(0);

  for (i = 0; i < AVI_audio_tracks(m_avi); i++)
    create_packetizer(i + 1);

  for (i = 0; static_cast<int>(m_subtitle_demuxers.size()) > i; ++i)
    create_subs_packetizer(i);
}

void
avi_reader_c::create_subs_packetizer(int idx) {
  auto &demuxer = m_subtitle_demuxers[idx];

  if (!demuxing_requested('s', 1 + AVI_audio_tracks(m_avi) + idx))
    return;

  m_ti.m_private_data.reset();

  demuxer.m_text_io = std::make_shared<mm_text_io_c>(std::make_shared<mm_mem_io_c>(*demuxer.m_subtitles));

  if (avi_subs_demuxer_t::TYPE_SRT == demuxer.m_type)
    create_srt_packetizer(idx);

  else if (avi_subs_demuxer_t::TYPE_SSA == demuxer.m_type)
    create_ssa_packetizer(idx);
}

void
avi_reader_c::create_srt_packetizer(int idx) {
  auto &demuxer  = m_subtitle_demuxers[idx];
  int id         = idx + 1 + AVI_audio_tracks(m_avi);

  auto parser    = std::make_shared<srt_parser_c>(demuxer.m_text_io, m_ti.m_fname, id);
  demuxer.m_subs = parser;

  parser->parse();

  auto need_recoding = demuxer.m_text_io->get_byte_order_mark() == byte_order_mark_e::none;
  demuxer.m_ptzr     = add_packetizer(new textsubs_packetizer_c(this, m_ti, MKV_S_TEXTUTF8, need_recoding));

  show_packetizer_info(id, ptzr(demuxer.m_ptzr));
}

void
avi_reader_c::create_ssa_packetizer(int idx) {
  auto &demuxer  = m_subtitle_demuxers[idx];
  int id         = idx + 1 + AVI_audio_tracks(m_avi);

  auto parser    = std::make_shared<ssa_parser_c>(*this, demuxer.m_text_io, m_ti.m_fname, id);
  demuxer.m_subs = parser;

  auto cc_utf8   = mtx::includes(m_ti.m_sub_charsets, id)                              ? charset_converter_c::init(m_ti.m_sub_charsets[id])
                 : mtx::includes(m_ti.m_sub_charsets, -1)                              ? charset_converter_c::init(m_ti.m_sub_charsets[-1])
                 : demuxer.m_text_io->get_byte_order_mark() != byte_order_mark_e::none ? charset_converter_c::init("UTF-8")
                 :                                                                       g_cc_local_utf8;

  parser->set_charset_converter(cc_utf8);
  parser->set_attachment_id_base(g_attachments.size());
  parser->parse();

  m_ti.m_private_data = memory_c::clone(parser->get_global());
  demuxer.m_ptzr      = add_packetizer(new textsubs_packetizer_c(this, m_ti, parser->is_ass() ?  MKV_S_TEXTASS : MKV_S_TEXTSSA));

  show_packetizer_info(id, ptzr(demuxer.m_ptzr));
}

void
avi_reader_c::add_audio_demuxer(int aid) {
  for (auto &demuxer : m_audio_demuxers)
    if (demuxer.m_aid == aid) // Demuxer already added?
      return;

  AVI_set_audio_track(m_avi, aid);
  if (AVI_read_audio_chunk(m_avi, nullptr) < 0) {
    mxwarn(fmt::format(FY("Could not find an index for audio track {0} (avilib error message: {1}). Skipping track.\n"), aid + 1, AVI_strerror()));
    return;
  }

  avi_demuxer_t demuxer;
  generic_packetizer_c *packetizer = nullptr;
  alWAVEFORMATEX *wfe              = m_avi->wave_format_ex[aid];
  uint32_t audio_format            = AVI_audio_format(m_avi);

  demuxer.m_aid                    = aid;
  demuxer.m_ptzr                   = -1;
  demuxer.m_samples_per_second     = AVI_audio_rate(m_avi);
  demuxer.m_channels               = AVI_audio_channels(m_avi);
  demuxer.m_bits_per_sample        = AVI_audio_bits(m_avi);

  m_ti.m_id                        = aid + 1;       // ID for this audio track.

  auto stream_header               = &m_avi->stream_headers[aid];
  auto dw_scale                    = static_cast<int64_t>(get_uint32_le(&stream_header->dw_scale));
  auto dw_rate                     = static_cast<int64_t>(get_uint32_le(&stream_header->dw_rate));
  auto dw_sample_size              = static_cast<int64_t>(get_uint32_le(&stream_header->dw_sample_size));
  m_ti.m_avi_audio_data_rate       = dw_scale ? dw_rate * dw_sample_size / dw_scale : 0;

  if ((0xfffe == audio_format) && (get_uint16_le(&wfe->cb_size) >= (sizeof(alWAVEFORMATEXTENSION)))) {
    alWAVEFORMATEXTENSIBLE *ext = reinterpret_cast<alWAVEFORMATEXTENSIBLE *>(wfe);
    audio_format                = get_uint32_le(&ext->extension.guid.data1);

  } else if (get_uint16_le(&wfe->cb_size) > 0)
    m_ti.m_private_data = memory_c::clone(wfe + 1, get_uint16_le(&wfe->cb_size));

  else
    m_ti.m_private_data.reset();

  demuxer.m_codec = codec_c::look_up_audio_format(audio_format);

  if (demuxer.m_codec.is(codec_c::type_e::A_PCM))
    packetizer = new pcm_packetizer_c(this, m_ti, demuxer.m_samples_per_second, demuxer.m_channels, demuxer.m_bits_per_sample, 0x0003 == audio_format ? pcm_packetizer_c::ieee_float : pcm_packetizer_c::little_endian_integer);

  else if (demuxer.m_codec.is(codec_c::type_e::A_MP2) || demuxer.m_codec.is(codec_c::type_e::A_MP3))
    packetizer = new mp3_packetizer_c(this, m_ti, demuxer.m_samples_per_second, demuxer.m_channels, false);

  else if (demuxer.m_codec.is(codec_c::type_e::A_AC3))
    packetizer = new ac3_packetizer_c(this, m_ti, demuxer.m_samples_per_second, demuxer.m_channels, 0);

  else if (demuxer.m_codec.is(codec_c::type_e::A_DTS))
    packetizer = create_dts_packetizer(aid);

  else if (demuxer.m_codec.is(codec_c::type_e::A_AAC))
    packetizer = create_aac_packetizer(aid, demuxer);

  else if (demuxer.m_codec.is(codec_c::type_e::A_VORBIS))
    packetizer = create_vorbis_packetizer(aid);

  else
    mxerror_tid(m_ti.m_fname, aid + 1, fmt::format(FY("Unknown/unsupported audio format 0x{0:04x} for this audio track.\n"), audio_format));

  packetizer->enable_avi_audio_sync(true);

  show_packetizer_info(aid + 1, *packetizer);

  demuxer.m_ptzr = add_packetizer(packetizer);

  m_audio_demuxers.push_back(demuxer);

  int i, maxchunks = AVI_audio_chunks(m_avi);
  for (i = 0; i < maxchunks; i++) {
    auto size = AVI_audio_size(m_avi, i);
    if (size < AVI_MAX_AUDIO_CHUNK_SIZE)
      m_bytes_to_process += size;
  }
}

generic_packetizer_c *
avi_reader_c::create_aac_packetizer(int aid,
                                    avi_demuxer_t &demuxer) {
  mtx::aac::audio_config_t audio_config;

  bool headerless = (AVI_audio_format(m_avi) != 0x706d);

  if (!m_ti.m_private_data
      || (   !headerless
          && ((sizeof(alWAVEFORMATEX) + 7)  < m_ti.m_private_data->get_size()))) {
    audio_config.channels             = AVI_audio_channels(m_avi);
    audio_config.sample_rate          = AVI_audio_rate(m_avi);
    if (44100 > audio_config.sample_rate) {
      audio_config.profile            = mtx::aac::PROFILE_SBR;
      audio_config.output_sample_rate = audio_config.sample_rate * 2;
      audio_config.sbr                = true;
    } else {
      audio_config.profile            = mtx::aac::PROFILE_MAIN;
      audio_config.output_sample_rate = audio_config.sample_rate;
      audio_config.sbr                = false;
    }

    m_ti.m_private_data = mtx::aac::create_audio_specific_config(audio_config);

  } else {
    auto parsed_audio_config = mtx::aac::parse_audio_specific_config(m_ti.m_private_data->get_buffer(), m_ti.m_private_data->get_size());
    if (!parsed_audio_config)
      mxerror_tid(m_ti.m_fname, aid + 1, Y("This AAC track does not contain valid headers. Could not parse the AAC information.\n"));

    audio_config = *parsed_audio_config;

    if (audio_config.sbr)
      audio_config.profile = mtx::aac::PROFILE_SBR;
  }

  demuxer.m_samples_per_second = audio_config.sample_rate;
  demuxer.m_channels           = audio_config.channels;

  auto packetizer              = new aac_packetizer_c(this, m_ti, audio_config, headerless ? aac_packetizer_c::headerless : aac_packetizer_c::with_headers);

  if (audio_config.sbr)
    packetizer->set_audio_output_sampling_freq(audio_config.output_sample_rate);

  return packetizer;
}

generic_packetizer_c *
avi_reader_c::create_dts_packetizer(int aid) {
  try {
    AVI_set_audio_track(m_avi, aid);

    long audio_position   = AVI_get_audio_position_index(m_avi);
    unsigned int num_read = 0;
    int dts_position      = -1;
    mtx::bytes::buffer_c buffer;
    mtx::dts::header_t dtsheader;

    while ((-1 == dts_position) && (10 > num_read)) {
      int chunk_size = AVI_read_audio_chunk(m_avi, nullptr);

      if (0 < chunk_size) {
        memory_cptr chunk = memory_c::alloc(chunk_size);
        AVI_read_audio_chunk(m_avi, reinterpret_cast<char *>(chunk->get_buffer()));

        buffer.add(*chunk);
        dts_position = mtx::dts::find_header(buffer.get_buffer(), buffer.get_size(), dtsheader);

      } else {
        dts_position = mtx::dts::find_header(buffer.get_buffer(), buffer.get_size(), dtsheader, true);
        break;
      }
    }

    if (-1 == dts_position)
      throw false;

    AVI_set_audio_position_index(m_avi, audio_position);

    return new dts_packetizer_c(this, m_ti, dtsheader);

  } catch (...) {
    mxerror_tid(m_ti.m_fname, aid + 1, Y("Could not find valid DTS headers in this track's first frames.\n"));
    return nullptr;
  }
}

generic_packetizer_c *
avi_reader_c::create_vorbis_packetizer(int aid) {
  try {
    if (m_ti.m_private_data)
      throw mtx::input::extended_x(Y("Invalid Vorbis headers in AVI audio track."));

    auto c = m_ti.m_private_data->get_buffer();

    if (2 != c[0])
      throw mtx::input::extended_x(Y("Invalid Vorbis headers in AVI audio track."));

    int offset           = 1;
    const int laced_size = m_ti.m_private_data->get_size();
    int i;

    std::vector<unsigned int> header_sizes;

    for (i = 0; 2 > i; ++i) {
      int size = 0;

      while ((offset < laced_size) && (255u == c[offset])) {
        size += 255;
        ++offset;
      }
      if ((laced_size - 1) <= offset)
        throw mtx::input::extended_x(Y("Invalid Vorbis headers in AVI audio track."));

      size            += c[offset];
      header_sizes[i]  = size;
      ++offset;
    }

    header_sizes.push_back(laced_size - offset - header_sizes[0] - header_sizes[1]);

    std::vector<memory_cptr> headers;
    headers.emplace_back(memory_c::borrow(&c[offset],                                     header_sizes[0]));
    headers.emplace_back(memory_c::borrow(&c[offset] + header_sizes[0],                   header_sizes[1]));
    headers.emplace_back(memory_c::borrow(&c[offset] + header_sizes[0] + header_sizes[1], header_sizes[2]));

    m_ti.m_private_data.reset();

    return new vorbis_packetizer_c(this, m_ti, headers);

  } catch (mtx::exception &e) {
    mxerror_tid(m_ti.m_fname, aid + 1, fmt::format("{0}\n", e.error()));

    // Never reached, but make the compiler happy:
    return nullptr;
  }
}

void
avi_reader_c::set_avc_nal_size_size(avc_es_video_packetizer_c *packetizer) {
  m_avc_nal_size_size = packetizer->get_nalu_size_length();

  for (int i = 0; i < static_cast<int>(m_max_video_frames); ++i) {
    int size = AVI_frame_size(m_avi, i);
    if (0 == size)
      continue;

    memory_cptr buffer = memory_c::alloc(size);

    AVI_set_video_position(m_avi, i);
    int key = 0;
    size    = AVI_read_frame(m_avi, reinterpret_cast<char *>(buffer->get_buffer()), &key);

    if (   (4 <= size)
        && (   (get_uint32_be(buffer->get_buffer()) == mtx::xyzvc::NALU_START_CODE)
            || (get_uint24_be(buffer->get_buffer()) == mtx::xyzvc::NALU_START_CODE)))
      m_avc_nal_size_size = -1;

    break;
  }

  AVI_set_video_position(m_avi, 0);
}

file_status_e
avi_reader_c::read_video() {
  if (m_video_frames_read >= m_max_video_frames)
    return flush_packetizer(m_vptzr);

  memory_cptr chunk;
  int key                   = 0;
  int old_video_frames_read = m_video_frames_read;

  int size, num_read;

  int dropped_frames_here   = 0;

  do {
    size  = AVI_frame_size(m_avi, m_video_frames_read);
    chunk = memory_c::alloc(size);
    num_read = AVI_read_frame(m_avi, reinterpret_cast<char *>(chunk->get_buffer()), &key);

    ++m_video_frames_read;

    if (0 > num_read) {
      // Error reading the frame: abort
      m_video_frames_read = m_max_video_frames;
      return flush_packetizer(m_vptzr);

    } else if (0 == num_read)
      ++dropped_frames_here;

  } while ((0 == num_read) && (m_video_frames_read < m_max_video_frames));

  if (0 == num_read)
    // This is only the case if the AVI contains dropped frames only.
    return flush_packetizer(m_vptzr);

  size_t i;
  for (i = m_video_frames_read; i < m_max_video_frames; ++i) {
    if (0 != AVI_frame_size(m_avi, i))
      break;

    int dummy_key;
    AVI_read_frame(m_avi, nullptr, &dummy_key);
    ++dropped_frames_here;
    ++m_video_frames_read;
  }

  auto timestamp          = mtx::to_int_rounded(old_video_frames_read     * m_default_duration);
  auto duration           = mtx::to_int_rounded((dropped_frames_here + 1) * m_default_duration);

  m_dropped_video_frames += dropped_frames_here;

  // AVC with framed packets (without NALU start codes but with length fields)
  // or non-AVC video track?
  if (0 >= m_avc_nal_size_size)
    ptzr(m_vptzr).process(std::make_shared<packet_t>(chunk, timestamp, duration, key ? VFT_IFRAME : VFT_PFRAMEAUTOMATIC, VFT_NOBFRAME));

  else {
    // AVC video track without NALU start codes. Re-frame with NALU start codes.
    int offset = 0;

    while ((offset + m_avc_nal_size_size) < num_read) {
      auto nalu_size  = get_uint_be(chunk->get_buffer() + offset, m_avc_nal_size_size);
      offset         += m_avc_nal_size_size;

      if ((nalu_size > static_cast<unsigned int>(num_read)) || ((offset + nalu_size) > static_cast<unsigned int>(num_read)))
        break;

      memory_cptr nalu = memory_c::alloc(4 + nalu_size);
      put_uint32_be(nalu->get_buffer(), mtx::xyzvc::NALU_START_CODE);
      memcpy(nalu->get_buffer() + 4, chunk->get_buffer() + offset, nalu_size);
      offset += nalu_size;

      ptzr(m_vptzr).process(std::make_shared<packet_t>(nalu, timestamp, duration, key ? VFT_IFRAME : VFT_PFRAMEAUTOMATIC, VFT_NOBFRAME));
    }
  }

  m_bytes_processed += num_read;

  return m_video_frames_read >= m_max_video_frames ? flush_packetizer(m_vptzr) :  FILE_STATUS_MOREDATA;
}

file_status_e
avi_reader_c::read_audio(avi_demuxer_t &demuxer) {
  AVI_set_audio_track(m_avi, demuxer.m_aid);

  while (true) {
    int size = AVI_audio_size(m_avi, AVI_get_audio_position_index(m_avi));

    // -1 indicates the last chunk.
    if (-1 == size)
      return flush_packetizer(demuxer.m_ptzr);

    // Sanity check. Ignore chunks with obvious wrong size information
    // (> 10 MB). Also skip 0-sized blocks. Those are officially
    // skipped.
    if (!size || (size > AVI_MAX_AUDIO_CHUNK_SIZE)) {
      AVI_set_audio_position_index(m_avi, AVI_get_audio_position_index(m_avi) + 1);
      continue;
    }

    auto chunk = memory_c::alloc(size);
    size       = AVI_read_audio_chunk(m_avi, reinterpret_cast<char *>(chunk->get_buffer()));

    if (0 > size)
      return flush_packetizer(demuxer.m_ptzr);

    if (!size)
      continue;

    ptzr(demuxer.m_ptzr).process(std::make_shared<packet_t>(chunk));

    m_bytes_processed += size;

    return AVI_get_audio_position_index(m_avi) < AVI_max_audio_chunk(m_avi) ? FILE_STATUS_MOREDATA : flush_packetizer(demuxer.m_ptzr);
  }
}

file_status_e
avi_reader_c::read_subtitles(avi_subs_demuxer_t &demuxer) {
  if (!demuxer.m_subs->empty())
    demuxer.m_subs->process(&ptzr(demuxer.m_ptzr));

  return demuxer.m_subs->empty() ? flush_packetizer(demuxer.m_ptzr) : FILE_STATUS_MOREDATA;
}

file_status_e
avi_reader_c::read(generic_packetizer_c *packetizer,
                   bool) {
  if ((-1 != m_vptzr) && (&ptzr(m_vptzr) == packetizer))
    return read_video();

  for (auto &demuxer : m_audio_demuxers)
    if ((-1 != demuxer.m_ptzr) && (&ptzr(demuxer.m_ptzr) == packetizer))
      return read_audio(demuxer);

  for (auto &subs_demuxer : m_subtitle_demuxers)
    if ((-1 != subs_demuxer.m_ptzr) && (&ptzr(subs_demuxer.m_ptzr) == packetizer))
      return read_subtitles(subs_demuxer);

  return flush_packetizers();
}

int64_t
avi_reader_c::get_progress() {
  return 0 == m_bytes_to_process ? m_in->getFilePointer() : m_bytes_processed;
}

int64_t
avi_reader_c::get_maximum_progress() {
  return 0 == m_bytes_to_process ? m_in->get_size() : m_bytes_to_process;
}

void
avi_reader_c::extended_identify_mpeg4_l2(mtx::id::info_c &info) {
  int size = AVI_frame_size(m_avi, 0);
  if (0 >= size)
    return;

  memory_cptr af_buffer = memory_c::alloc(size);
  uint8_t *buffer = af_buffer->get_buffer();
  int dummy_key;

  AVI_read_frame(m_avi, reinterpret_cast<char *>(buffer), &dummy_key);

  uint32_t par_num, par_den;
  if (mtx::mpeg4_p2::extract_par(buffer, size, par_num, par_den)) {
    auto aspect_ratio = mtx::rational(m_video_width * par_num, m_video_height * par_den);

    int disp_width, disp_height;
    if (aspect_ratio > mtx::rational(m_video_width, m_video_height)) {
      disp_width  = mtx::to_int_rounded(m_video_height * aspect_ratio);
      disp_height = m_video_height;

    } else {
      disp_width  = m_video_width;
      disp_height = mtx::to_int_rounded(m_video_width / aspect_ratio);
    }

    info.add_joined(mtx::id::display_dimensions, "x"s, disp_width, disp_height);
  }
}

void
avi_reader_c::identify() {
  id_result_container();
  identify_video();
  identify_audio();
  identify_subtitles();
  identify_attachments();
}

void
avi_reader_c::identify_video() {
  if (!m_video_track_ok)
    return;

  auto info       = mtx::id::info_c{};
  auto codec      = codec_c::look_up(AVI_video_compressor(m_avi));
  auto fourcc_str = fourcc_c{AVI_video_compressor(m_avi)}.description();

  if (codec.is(codec_c::type_e::V_MPEG4_P2))
    extended_identify_mpeg4_l2(info);

  else if (codec.is(codec_c::type_e::V_MPEG4_P10))
    info.add(mtx::id::packetizer, mtx::id::mpeg4_p10_es_video);

  info.add_joined(mtx::id::pixel_dimensions,   "x"s, m_video_width, m_video_height);
  info.add_joined(mtx::id::display_dimensions, "x"s, m_video_display_width, m_video_display_height);

  id_result_track(0, ID_RESULT_TRACK_VIDEO, codec.get_name(fourcc_str), info.get());
}

void
avi_reader_c::identify_audio() {
  int i;
  for (i = 0; i < AVI_audio_tracks(m_avi); i++) {
    AVI_set_audio_track(m_avi, i);

    auto info = mtx::id::info_c{};

    info.add(mtx::id::audio_channels,           AVI_audio_channels(m_avi));
    info.add(mtx::id::audio_sampling_frequency, AVI_audio_rate(m_avi));
    info.add(mtx::id::audio_bits_per_sample,    AVI_audio_bits(m_avi));

    unsigned int audio_format = AVI_audio_format(m_avi);
    alWAVEFORMATEX *wfe       = m_avi->wave_format_ex[i];
    if ((0xfffe == audio_format) && (get_uint16_le(&wfe->cb_size) >= (sizeof(alWAVEFORMATEXTENSION)))) {
      alWAVEFORMATEXTENSIBLE *ext = reinterpret_cast<alWAVEFORMATEXTENSIBLE *>(wfe);
      audio_format = get_uint32_le(&ext->extension.guid.data1);
    }

    auto codec = codec_c::look_up_audio_format(audio_format);
    id_result_track(i + 1, ID_RESULT_TRACK_AUDIO, codec.get_name(fmt::format("unsupported (0x{0:04x})", audio_format)), info.get());
  }
}

void
avi_reader_c::identify_subtitles() {
  size_t i;
  for (i = 0; m_subtitle_demuxers.size() > i; ++i) {
    auto info = mtx::id::info_c{};
    if (   (avi_subs_demuxer_t::TYPE_SRT == m_subtitle_demuxers[i].m_type)
        || (avi_subs_demuxer_t::TYPE_SSA == m_subtitle_demuxers[i].m_type))
      info.add(mtx::id::text_subtitles, true);

    if (m_subtitle_demuxers[i].m_encoding)
      info.add(mtx::id::encoding, *m_subtitle_demuxers[i].m_encoding);

    id_result_track(1 + AVI_audio_tracks(m_avi) + i, ID_RESULT_TRACK_SUBTITLES,
                      avi_subs_demuxer_t::TYPE_SRT == m_subtitle_demuxers[i].m_type ? codec_c::get_name(codec_c::type_e::S_SRT, "SRT")
                    : avi_subs_demuxer_t::TYPE_SSA == m_subtitle_demuxers[i].m_type ? codec_c::get_name(codec_c::type_e::S_SSA_ASS, "SSA/ASS")
                    :                                                                 "unknown",
                    info.get());
  }
}

void
avi_reader_c::identify_attachments() {
  size_t i;

  for (i = 0; m_subtitle_demuxers.size() > i; ++i) {
    try {
      auto &demuxer = m_subtitle_demuxers[i];
      auto text_io  = std::make_shared<mm_text_io_c>(std::make_shared<mm_mem_io_c>(*demuxer.m_subtitles));
      auto parser   = std::make_shared<ssa_parser_c>(*this, text_io, m_ti.m_fname, i + 1 + AVI_audio_tracks(m_avi));

      parser->set_attachment_id_base(g_attachments.size());
      parser->parse();

    } catch (...) {
    }
  }

  for (auto const &attachment : g_attachments)
    id_result_attachment(attachment->ui_id, attachment->mime_type, attachment->data->get_size(), attachment->name, attachment->description);
}

void
avi_reader_c::add_available_track_ids() {
  // Yes, '>=' is correct. Don't forget the video track!
  add_available_track_id_range(AVI_audio_tracks(m_avi) + m_subtitle_demuxers.size() + 1);
}

void
avi_reader_c::debug_dump_video_index() {
  int num_video_frames = AVI_video_frames(m_avi), i;

  mxinfo(fmt::format("AVI video index dump: {0} entries; default duration: {1}\n", num_video_frames, m_default_duration));
  for (i = 0; num_video_frames > i; ++i) {
    int key = 0;
    AVI_read_frame(m_avi, nullptr, &key);
    mxinfo(fmt::format("  {0}: {1} bytes; key: {2}\n", i, AVI_frame_size(m_avi, i), key));
  }

  AVI_set_video_position(m_avi, 0);
}
