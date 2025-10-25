/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   MPEG TS (transport stream) demultiplexer module

   Written by Massimo Callegari <massimocallegari@yahoo.it>
   and Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <iostream>
#include <iterator>
#include <optional>
#include <type_traits>

#include "common/at_scope_exit.h"
#include "common/avc/es_parser.h"
#include "common/avc/types.h"
#include "common/bit_reader.h"
#include "common/bluray/clpi.h"
#include "common/bluray/mpls.h"
#include "common/bluray/util.h"
#include "common/bswap.h"
#include "common/chapters/bluray.h"
#include "common/checksums/crc.h"
#include "common/checksums/base_fwd.h"
#include "common/dovi_meta.h"
#include "common/endian.h"
#include "common/hacks.h"
#include "common/hdmv_textst.h"
#include "common/mm_io_x.h"
#include "common/mp3.h"
#include "common/mm_file_io.h"
#include "common/mm_mpls_multi_file_io.h"
#include "common/ac3.h"
#include "common/id_info.h"
#include "common/list_utils.h"
#include "common/mm_mem_io.h"
#include "common/mpeg1_2.h"
#include "common/mpeg4_p2.h"
#include "common/path.h"
#include "common/qt.h"
#include "common/strings/formatting.h"
#include "input/aac_framing_packet_converter.h"
#include "input/bluray_pcm_channel_layout_packet_converter.h"
#include "input/dvbsub_pes_framing_removal_packet_converter.h"
#include "input/hevc_dovi_layer_combiner_packet_converter.h"
#include "input/r_mpeg_ts.h"
#include "input/teletext_to_srt_packet_converter.h"
#include "input/truehd_ac3_splitting_packet_converter.h"
#include "merge/cluster_helper.h"
#include "merge/output_control.h"
#include "merge/packet.h"
#include "output/p_aac.h"
#include "output/p_ac3.h"
#include "output/p_avc_es.h"
#include "output/p_dts.h"
#include "output/p_dvbsub.h"
#include "output/p_hdmv_pgs.h"
#include "output/p_hdmv_textst.h"
#include "output/p_hevc_es.h"
#include "output/p_mp3.h"
#include "output/p_mpeg1_2.h"
#include "output/p_pcm.h"
#include "output/p_textsubs.h"
#include "output/p_truehd.h"
#include "output/p_vc1.h"

// This is ISO/IEC 13818-1.

namespace mtx::mpeg_ts {

constexpr auto TS_PACKET_SIZE     = 188;
constexpr auto TS_MAX_PACKET_SIZE = 204;

constexpr auto TS_PAT_PID         = 0x0000;
constexpr auto TS_SDT_PID         = 0x0011;

constexpr auto TS_SDT_TID         = 0x42;

int reader_c::potential_packet_sizes[] = { 188, 192, 204, 0 };

// ------------------------------------------------------------

track_c::track_c(reader_c &p_reader,
                 pid_type_e p_type)
  : reader{p_reader}
  , m_file_num{p_reader.m_current_file}
  , m_id{}
  , processed{}
  , type{p_type}
  , pid{}
  , program_number{}
  , pes_payload_size_to_read{}
  , pes_payload_read{new mtx::bytes::buffer_c}
  , probed_ok{}
  , ptzr{-1}
  , m_timestamp_wrap_add{timestamp_c::ns(0)}
  , m_subtitle_timestamp_correction{timestamp_c::ns(0)}
  , v_interlaced{}
  , v_version{}
  , v_width{}
  , v_height{}
  , v_dwidth{}
  , v_dheight{}
  , a_channels{}
  , a_sample_rate{}
  , a_bits_per_sample{}
  , a_bsid{}
  , m_aac_multiplex_type{aac::parser_c::unknown_multiplex}
  , m_apply_dts_timestamp_fix{}
  , m_use_dts{}
  , m_timestamps_wrapped{}
  , m_truehd_found_truehd{}
  , m_truehd_found_ac3{}
  , m_master{}
  , skip_packet_data_bytes{}
  , m_debug_delivery{}
  , m_debug_timestamp_wrapping{}
  , m_debug_headers{"mpeg_ts|mpeg_ts_headers"}
{
}

void
track_c::process(packet_cptr const &packet) {
  if (!converter || !converter->convert_for_pid(packet, pid))
    reader.m_reader_packetizers[ptzr]->process(packet);
}

void
track_c::send_to_packetizer() {
  auto &f                 = reader.file();
  auto timestamp_to_use   = !m_timestamp.valid()                                               ? timestamp_c{}
                          : reader.m_dont_use_audio_pts && (pid_type_e::audio == type)         ? timestamp_c{}
                          : m_apply_dts_timestamp_fix && (m_previous_timestamp == m_timestamp) ? timestamp_c{}
                          :                                                                      std::max(m_timestamp, f.m_global_timestamp_offset);

  auto timestamp_to_check = f.m_stream_timestamp.valid() ? f.m_stream_timestamp : timestamp_c::ns(0);
  auto const &min         = f.m_timestamp_restriction_min;
  auto const &max         = f.m_timestamp_restriction_max;
  auto use_packet         = (ptzr != -1) || converter;
  auto bytes_to_skip      = std::min<size_t>(pes_payload_read->get_size(), skip_packet_data_bytes);

  if (   (min.valid() && (timestamp_to_check <  min) && !f.m_timestamp_restriction_min_seen)
      || (max.valid() && (timestamp_to_check >= max)))
    use_packet = false;

  if (use_packet && !f.m_timestamp_restriction_min_seen && min.valid())
    f.m_timestamp_restriction_min_seen = true;

  if (timestamp_to_use.valid() && f.m_global_timestamp_offset.valid())
    timestamp_to_use -= std::min(timestamp_to_use, f.m_global_timestamp_offset);

  mxdebug_if(m_debug_delivery,
             fmt::format("send_to_packetizer: PID {0} expected {1} actual {2} timestamp_to_use {3} timestamp_to_check {4} m_timestamp {5} m_previous_timestamp {6} stream_timestamp {7} restriction {8}/{9} min_seen {10} ptzr {11} use? {12}\n",
                         pid, pes_payload_size_to_read, pes_payload_read->get_size() - bytes_to_skip, timestamp_to_use, timestamp_to_check, m_timestamp, m_previous_timestamp, f.m_stream_timestamp, min, max, f.m_timestamp_restriction_min_seen, ptzr, use_packet));

  if (use_packet) {
    process(std::make_shared<packet_t>(memory_c::clone(pes_payload_read->get_buffer() + bytes_to_skip, pes_payload_read->get_size() - bytes_to_skip), timestamp_to_use.to_ns(-1)));

    f.m_packet_sent_to_packetizer = true;
  }

  clear_pes_payload();
  processed            = false;
  m_previous_timestamp = m_timestamp;
  m_timestamp.reset();
}

bool
track_c::is_pes_payload_complete()
  const {
  return !is_pes_payload_size_unbounded()
      && pes_payload_size_to_read
      && !remaining_payload_size_to_read();
}

bool
track_c::is_pes_payload_size_unbounded()
  const {
  return pes_payload_size_to_read == offsetof(pes_header_t, flags1);
}

void
track_c::add_pes_payload(uint8_t *ts_payload,
                         size_t ts_payload_size) {
  auto to_add = is_pes_payload_size_unbounded() ? ts_payload_size : std::min(ts_payload_size, remaining_payload_size_to_read());

  if (to_add)
    pes_payload_read->add(ts_payload, to_add);
}

void
track_c::add_pes_payload_to_probe_data() {
  if (!m_probe_data)
    m_probe_data = mtx::bytes::buffer_cptr(new mtx::bytes::buffer_c);
  m_probe_data->add(pes_payload_read->get_buffer(), pes_payload_read->get_size());
}

void
track_c::clear_pes_payload() {
  pes_payload_read->clear();
  pes_payload_size_to_read = 0;
}

int
track_c::new_stream_v_mpeg_1_2(bool end_of_detection) {
  if (!m_m2v_parser) {
    m_m2v_parser = std::shared_ptr<M2VParser>(new M2VParser);
    m_m2v_parser->SetProbeMode();
    m_m2v_parser->SetThrowOnError(true);
  }

  m_m2v_parser->WriteData(pes_payload_read->get_buffer(), pes_payload_read->get_size());
  if (end_of_detection)
    m_m2v_parser->SetEOS();

  int state = m_m2v_parser->GetState();
  if (state != MPV_PARSER_STATE_FRAME) {
    mxdebug_if(m_debug_headers, fmt::format("new_stream_v_mpeg_1_2: no valid frame in {0} bytes\n", pes_payload_read->get_size()));
    return FILE_STATUS_MOREDATA;
  }

  MPEG2SequenceHeader seq_hdr = m_m2v_parser->GetSequenceHeader();
  std::shared_ptr<MPEGFrame> frame(m_m2v_parser->ReadFrame());
  if (!frame)
    return FILE_STATUS_MOREDATA;

  codec          = codec_c::look_up(codec_c::type_e::V_MPEG12);
  v_interlaced   = !seq_hdr.progressiveSequence;
  v_version      = m_m2v_parser->GetMPEGVersion();
  v_width        = seq_hdr.width;
  v_height       = seq_hdr.height;
  v_aspect_ratio = seq_hdr.aspectRatio;

  m_default_duration = 1'000'000'000 / seq_hdr.frameRate;

  if ((0 >= v_aspect_ratio) || (1 == v_aspect_ratio))
    v_dwidth = v_width;
  else
    v_dwidth = mtx::to_int_rounded(v_height * v_aspect_ratio);
  v_dheight  = v_height;

  mxdebug_if(m_debug_headers, fmt::format("new_stream_v_mpeg_1_2: width: {0}, height: {1}\n", v_width, v_height));
  if (v_width == 0 || v_height == 0)
    return FILE_STATUS_MOREDATA;

  return 0;
}

int
track_c::new_stream_v_avc(bool end_of_detection) {
  if (!m_avc_parser)
    m_avc_parser = std::make_shared<mtx::avc::es_parser_c>();

  mxdebug_if(m_debug_headers, fmt::format("new_stream_v_avc: packet size: {0}\n", pes_payload_read->get_size()));

  m_avc_parser->add_bytes(pes_payload_read->get_buffer(), pes_payload_read->get_size());
  if (end_of_detection)
    m_avc_parser->flush();

  if (!m_avc_parser->headers_parsed())
    return FILE_STATUS_MOREDATA;

  codec    = codec_c::look_up(codec_c::type_e::V_MPEG4_P10);
  v_width  = m_avc_parser->get_width();
  v_height = m_avc_parser->get_height();

  if (m_avc_parser->has_par_been_found()) {
    auto dimensions = m_avc_parser->get_display_dimensions();
    v_dwidth        = dimensions.first;
    v_dheight       = dimensions.second;
  }

  return 0;
}

int
track_c::new_stream_v_hevc(bool end_of_detection) {
  if (!m_hevc_parser)
    m_hevc_parser = std::make_shared<mtx::hevc::es_parser_c>();

  m_hevc_parser->add_bytes(pes_payload_read->get_buffer(), pes_payload_read->get_size());
  if (end_of_detection)
    m_hevc_parser->flush();

  if (!m_hevc_parser->headers_parsed())
    return FILE_STATUS_MOREDATA;

  codec    = codec_c::look_up(codec_c::type_e::V_MPEGH_P2);
  v_width  = m_hevc_parser->get_width();
  v_height = m_hevc_parser->get_height();

  if (m_hevc_parser->has_par_been_found()) {
    auto dimensions = m_hevc_parser->get_display_dimensions();
    v_dwidth        = dimensions.first;
    v_dheight       = dimensions.second;
  }

  if (!m_hevc_parser->has_dovi_rpu_header()) {
    mxdebug_if(reader.m_debug_dovi, fmt::format("new_stream_v_hevc: no DOVI RPU found\n"));
    return 0;
  }

  auto hdr      = m_hevc_parser->get_dovi_rpu_header();
  auto vui      = m_hevc_parser->get_vui_info();
  auto duration = m_hevc_parser->has_stream_default_duration() ? m_hevc_parser->get_stream_default_duration() : m_hevc_parser->get_most_often_used_duration();

  m_dovi_config = create_dovi_configuration_record(hdr, v_width, v_height, vui, duration);

  if (reader.m_debug_dovi) {
    mxdebug(fmt::format("new_stream_v_hevc: PID {0} DOVI decoder configuration record parsed; dumping:\n", pid));
    m_dovi_config->dump();
  }

  return 0;
}

int
track_c::new_stream_v_vc1() {
  if (!m_vc1_parser)
    m_vc1_parser = std::make_shared<mtx::vc1::es_parser_c>();

  m_vc1_parser->add_bytes(pes_payload_read->get_buffer(), pes_payload_read->get_size());

  if (!m_vc1_parser->is_sequence_header_available())
    return FILE_STATUS_MOREDATA;

  auto seqhdr = mtx::vc1::sequence_header_t{};
  m_vc1_parser->get_sequence_header(seqhdr);

  v_width  = seqhdr.pixel_width;
  v_height = seqhdr.pixel_height;

  return 0;
}

int
track_c::new_stream_a_mpeg() {
  add_pes_payload_to_probe_data();

  mp3_header_t header;

  int offset = find_mp3_header(m_probe_data->get_buffer(), m_probe_data->get_size());
  if (-1 == offset)
    return FILE_STATUS_MOREDATA;

  decode_mp3_header(m_probe_data->get_buffer() + offset, &header);
  a_channels    = header.channels;
  a_sample_rate = header.sampling_frequency;
  codec         = header.get_codec();

  mxdebug_if(m_debug_headers, fmt::format("new_stream_a_mpeg: Channels: {0}, sample rate: {1}\n",a_channels, a_sample_rate));
  return 0;
}

int
track_c::new_stream_a_aac() {
  add_pes_payload_to_probe_data();

  auto pos = aac::parser_c::find_consecutive_frames(m_probe_data->get_buffer(), m_probe_data->get_size(), 5);
  if (pos == -1)
    return FILE_STATUS_MOREDATA;

  auto parser = aac::parser_c{};
  parser.add_bytes(m_probe_data->get_buffer() + pos, m_probe_data->get_size() - pos);
  if (!parser.frames_available() || !parser.headers_parsed())
    return FILE_STATUS_MOREDATA;

  m_aac_frame          = parser.get_frame();
  m_aac_multiplex_type = parser.get_multiplex_type();
  a_channels           = m_aac_frame.m_header.config.channels;
  a_sample_rate        = m_aac_frame.m_header.config.sample_rate;

  mxdebug_if(reader.m_debug_aac,
             fmt::format("new_stream_a_aac: found headers at {0} multiplex type {1} first header: {2}\n",
                         pos, aac::parser_c::get_multiplex_type_name(m_aac_multiplex_type), m_aac_frame.to_string()));

  return 0;
}

int
track_c::new_stream_a_ac3() {
  add_pes_payload_to_probe_data();

  ac3::parser_c parser;
  parser.add_bytes(m_probe_data->get_buffer(), m_probe_data->get_size());
  if (!parser.frame_available())
    return FILE_STATUS_MOREDATA;

  ac3::frame_c header = parser.get_frame();

  mxdebug_if(m_debug_headers,
             fmt::format("new_stream_a_ac3: first ac3 header bsid {0} channels {1} sample_rate {2} bytes {3} samples {4}\n",
                         header.m_bs_id, header.m_channels, header.m_sample_rate, header.m_bytes, header.m_samples));

  a_channels    = header.m_channels;
  a_sample_rate = header.m_sample_rate;
  a_bsid        = header.m_bs_id;
  codec         = header.get_codec();

  return 0;
}

int
track_c::new_stream_a_dts() {
  add_pes_payload_to_probe_data();

  if (-1 == mtx::dts::find_header(m_probe_data->get_buffer(), m_probe_data->get_size(), a_dts_header))
    return FILE_STATUS_MOREDATA;

  m_apply_dts_timestamp_fix = true;
  a_channels                = a_dts_header.get_total_num_audio_channels();
  a_sample_rate             = a_dts_header.get_effective_sampling_frequency();

  codec.set_specialization(a_dts_header.get_codec_specialization());

  return 0;
}

int
track_c::new_stream_a_pcm() {
  static uint8_t const s_bits_per_samples[4] = { 0, 16, 20, 24 };
  static uint8_t const s_channels[16]        = { 0, 1, 0, 2, 3, 3, 4, 4, 5, 6, 7, 8, 0, 0, 0, 0 };

  add_pes_payload_to_probe_data();

  if (4 > m_probe_data->get_size())
    return FILE_STATUS_MOREDATA;

  skip_packet_data_bytes = 4;
  auto buffer            = m_probe_data->get_buffer();
  a_channels             = s_channels[         buffer[2] >> 4 ];
  a_bits_per_sample      = s_bits_per_samples[ buffer[3] >> 6 ];
  auto sample_rate_idx   = buffer[2] & 0x0f;
  a_sample_rate          = sample_rate_idx == 1 ?  48000
                         : sample_rate_idx == 4 ?  96000
                         : sample_rate_idx == 5 ? 192000
                         :                             0;

  mxdebug_if(m_debug_headers,
             fmt::format("new_stream_a_pcm: header: 0x{0:08x} channels: {1}, sample rate: {2}, bits per channel: {3}\n",
                         get_uint32_be(buffer), a_channels, a_sample_rate, a_bits_per_sample));

  return 0;
}

int
track_c::new_stream_a_truehd() {
  if (!m_truehd_parser)
    m_truehd_parser.reset(new mtx::truehd::parser_c);

  m_truehd_parser->add_data(pes_payload_read->get_buffer(), pes_payload_read->get_size());
  clear_pes_payload();

  while (m_truehd_parser->frame_available() && (!m_truehd_found_truehd || !m_truehd_found_ac3)) {
    auto frame = m_truehd_parser->get_next_frame();

    if (frame->is_ac3()) {
      if (!m_truehd_found_ac3) {
        m_truehd_found_ac3 = true;
        auto &header       = frame->m_ac3_header;

        mxdebug_if(m_debug_headers,
                   fmt::format("new_stream_a_truehd: first AC-3 header bsid {0} channels {1} sample_rate {2} bytes {3} samples {4}\n",
                               header.m_bs_id, header.m_channels, header.m_sample_rate, header.m_bytes, header.m_samples));

        auto &coupled_track         = *m_coupled_tracks[0];
        coupled_track.a_channels    = header.m_channels;
        coupled_track.a_sample_rate = header.m_sample_rate;
        coupled_track.a_bsid        = header.m_bs_id;
        coupled_track.probed_ok     = true;
      }

      continue;

    }

    if (!frame->is_sync())
      continue;

    mxdebug_if(m_debug_headers,
               fmt::format("new_stream_a_truehd: first TrueHD header channels {0} sampling_rate {1} samples_per_frame {2}\n",
                           frame->m_channels, frame->m_sampling_rate, frame->m_samples_per_frame));

    codec                 = frame->codec();
    a_channels            = frame->m_channels;
    a_sample_rate         = frame->m_sampling_rate;
    m_truehd_found_truehd = true;
  }

  return m_truehd_found_truehd && m_truehd_found_ac3 ? 0 : FILE_STATUS_MOREDATA;
}

int
track_c::new_stream_s_hdmv_textst() {
  add_pes_payload_to_probe_data();

  // CodecPrivate for TextST consists solely of the "dialog style segment" element:
  // segment descriptor:
  //   1 byte segment type (0x81 == dialog style segment)
  //   2 bytes segment size (Big Endian)
  // x bytes dialog style segment data

  if (!m_probe_data || (m_probe_data->get_size() < 3))
    return FILE_STATUS_MOREDATA;

  auto buf = m_probe_data->get_buffer();
  if (static_cast<mtx::hdmv_textst::segment_type_e>(buf[0]) != mtx::hdmv_textst::dialog_style_segment)
    return FILE_STATUS_DONE;

  auto dialog_segment_size = get_uint16_be(&buf[1]);
  if ((dialog_segment_size + 3u) > m_probe_data->get_size())
    return FILE_STATUS_MOREDATA;

  m_codec_private_data = memory_c::clone(buf, dialog_segment_size + 3);

  return 0;
}

int
track_c::new_stream_s_dvbsub() {
  if (!m_codec_private_data || (5 != m_codec_private_data->get_size()))
      return FILE_STATUS_DONE;

  return 0;
}

int
track_c::new_stream_s_teletext() {
  if (!m_ttx_parser)
    return FILE_STATUS_DONE;

  m_ttx_parser->convert(std::make_shared<packet_t>(memory_c::clone(pes_payload_read->get_buffer(), pes_payload_read->get_size())));
  clear_pes_payload();

  return FILE_STATUS_MOREDATA;
}

bool
track_c::has_packetizer()
  const {
  return -1 != ptzr;
}

void
track_c::set_pid(uint16_t new_pid) {
  pid = new_pid;

  std::string arg;
  m_debug_delivery = debugging_c::requested("mpeg_ts")
                  || (   debugging_c::requested("mpeg_ts_delivery", &arg)
                      && (arg.empty() || (arg == fmt::to_string(pid))));

  m_debug_timestamp_wrapping = debugging_c::requested("mpeg_ts")
                            || (   debugging_c::requested("mpeg_ts_timestamp_wrapping", &arg)
                                && (arg.empty() || (arg == fmt::to_string(pid))));
}

bool
track_c::detect_timestamp_wrap(timestamp_c const &timestamp)
  const {
  static auto const s_wrap_limit = timestamp_c::mpeg(1 << 30);

  if (   !timestamp.valid()
      || !m_previous_valid_timestamp.valid()
      || ((timestamp - m_previous_valid_timestamp).abs() < s_wrap_limit))
    return false;
  return true;
}

void
track_c::adjust_timestamp_for_wrap(timestamp_c &timestamp) {
  static auto const s_wrap_limit = timestamp_c::mpeg(1ll << 30);
  static auto const s_bad_limit  = timestamp_c::m(5);

  if (!timestamp.valid())
    return;

  if (timestamp < s_wrap_limit)
    timestamp += m_timestamp_wrap_add;

  // For subtitle tracks only detect jumps backwards in time, not
  // forward. Subtitles often have holes larger than five minutes
  // between the entries.
  if (   ((timestamp < m_previous_valid_timestamp) || (pid_type_e::subtitles != type))
      && ((timestamp - m_previous_valid_timestamp).abs() >= s_bad_limit))
    timestamp = timestamp_c{};
}

void
track_c::handle_timestamp_wrap(timestamp_c &pts,
                               timestamp_c &dts) {
  static auto const s_wrap_add    = timestamp_c::mpeg(1ll << 33);
  static auto const s_wrap_limit  = timestamp_c::mpeg(1ll << 30);
  static auto const s_reset_limit = timestamp_c::h(1);

  if (!m_timestamps_wrapped) {
    m_timestamps_wrapped = detect_timestamp_wrap(pts) || detect_timestamp_wrap(dts);
    if (m_timestamps_wrapped) {
      m_timestamp_wrap_add += s_wrap_add;
      mxdebug_if(m_debug_timestamp_wrapping,
                 fmt::format("handle_timestamp_wrap: Timestamp wrapping detected for PID {0} pts {1} dts {2} previous_valid {3} global_offset {4} new wrap_add {5}\n",
                             pid, pts, dts, m_previous_valid_timestamp, reader.file().m_global_timestamp_offset, m_timestamp_wrap_add));

    }

  } else if (pts.valid() && (pts < s_wrap_limit) && (pts > s_reset_limit)) {
    m_timestamps_wrapped = false;
    mxdebug_if(m_debug_timestamp_wrapping,
               fmt::format("handle_timestamp_wrap: Timestamp wrapping reset for PID {0} pts {1} dts {2} previous_valid {3} global_offset {4} current wrap_add {5}\n",
                           pid, pts, dts, m_previous_valid_timestamp, reader.file().m_global_timestamp_offset, m_timestamp_wrap_add));
  }

  adjust_timestamp_for_wrap(pts);
  adjust_timestamp_for_wrap(dts);

  // mxinfo(fmt::format("pid {4} PTS before {0} now {1} wrapped {2} reset prevvalid {3} diff {5}\n", before, pts, m_timestamps_wrapped, m_previous_valid_timestamp, pid, pts - m_previous_valid_timestamp));
}

drop_decision_e
track_c::handle_bogus_subtitle_timestamps(timestamp_c &pts,
                                          timestamp_c &dts) {
  auto &f = reader.file();

  if (processing_state_e::muxing != f.m_state)
    return drop_decision_e::keep;

  // Sometimes the subtitle timestamps aren't valid, they can be off
  // from the audio/video timestamps by hours. In such a case replace
  // the subtitle's timestamps with last valid one from audio/video
  // packets.
  if (   mtx::included_in(type, pid_type_e::audio, pid_type_e::video)
      && pts.valid()) {
    f.m_last_non_subtitle_pts = pts;
    f.m_last_non_subtitle_dts = dts;
    return drop_decision_e::keep;

  }

  if (pid_type_e::subtitles != type)
    return drop_decision_e::keep;

  // PGS subtitles often have their "stop displaying" segments located
  // right after the "display stuff" segments. In that case their PTS
  // will differ from the surrounding audio/video PTS
  // considerably. That's fine, though. No correction must take in
  // such a case.
  if (codec.is(codec_c::type_e::S_HDMV_PGS))
    return drop_decision_e::keep;

  if (pts.valid())
    pts += m_subtitle_timestamp_correction;
  if (dts.valid())
    dts += m_subtitle_timestamp_correction;

  if (!f.m_last_non_subtitle_pts.valid())
    return f.m_has_audio_or_video_track ? drop_decision_e::drop : drop_decision_e::keep;

  if (   !pts.valid()
      || ((pts - f.m_last_non_subtitle_pts).abs() >= timestamp_c::s(5))) {
    auto additional_correction       = f.m_last_non_subtitle_pts - pts;
    m_subtitle_timestamp_correction += additional_correction;
    pts                              = f.m_last_non_subtitle_pts;
    dts                              = f.m_last_non_subtitle_dts;

    mxdebug_if(m_debug_timestamp_wrapping, fmt::format("additional subtitle correction {0} total {1}\n", additional_correction, m_subtitle_timestamp_correction));
  }

  return drop_decision_e::keep;
}

bool
track_c::parse_ac3_pmt_descriptor(pmt_descriptor_t const &,
                                  pmt_pid_info_t const &pmt_pid_info) {
  if (pmt_pid_info.stream_type != stream_type_e::iso_13818_pes_private)
    return false;

  type  = pid_type_e::audio;
  codec = codec_c::look_up(codec_c::type_e::A_AC3);

  return true;
}

bool
track_c::parse_dovi_pmt_descriptor(pmt_descriptor_t const &pmt_descriptor,
                                   pmt_pid_info_t const &pmt_pid_info) {
  if (pmt_pid_info.stream_type != stream_type_e::iso_13818_pes_private)
    return false;

  mxdebug_if(reader.m_debug_dovi, fmt::format("parse_dovi_pmt_descriptor: parsing PMT descriptor of length {0}\n", pmt_descriptor.length));

  mtx::bits::reader_c r{reinterpret_cast<uint8_t const *>(&pmt_descriptor + 1), pmt_descriptor.length};

  try {
    mtx::dovi::dovi_decoder_configuration_record_t cfg;

    cfg.dv_version_major = r.get_bits(8);
    cfg.dv_version_minor = r.get_bits(8);
    cfg.dv_profile       = r.get_bits(7);
    cfg.dv_level         = r.get_bits(6);
    cfg.rpu_present_flag = r.get_bit();
    cfg.bl_present_flag  = r.get_bit();
    cfg.el_present_flag  = r.get_bit();

    if (!cfg.bl_present_flag) {
      m_dovi_base_layer_pid = r.get_bits(13);
      r.skip_bits(3);
    }

    cfg.dv_bl_signal_compatibility_id = r.get_bits(4);
    r.skip_bits(4);

    m_dovi_config = cfg;

    if (reader.m_debug_dovi) {
      std::string base_layer_pid;
      if (m_dovi_base_layer_pid)
        base_layer_pid = fmt::format("; base layer PID: {0}", *m_dovi_base_layer_pid);

      mxdebug(fmt::format("parse_dovi_pmt_descriptor: DOVI decoder configuration record parsed{0}; dumping:\n", base_layer_pid));
      cfg.dump();
    }

    return true;

  } catch (mtx::mm_io::exception const &) {
    mxdebug_if(reader.m_debug_dovi, fmt::format("parse_dovi_pmt_descriptor: I/O exception\n"));
    return false;

  } catch (...) {
    mxdebug_if(reader.m_debug_dovi, fmt::format("parse_dovi_pmt_descriptor: unknown exception\n"));
    return false;
  }
}

bool
track_c::parse_dts_pmt_descriptor(pmt_descriptor_t const &,
                                  pmt_pid_info_t const &pmt_pid_info) {
  if (pmt_pid_info.stream_type != stream_type_e::iso_13818_pes_private)
    return false;

  type  = pid_type_e::audio;
  codec = codec_c::look_up(codec_c::type_e::A_DTS);

  return true;
}

bool
track_c::parse_teletext_pmt_descriptor(pmt_descriptor_t const &pmt_descriptor,
                                       pmt_pid_info_t const &pmt_pid_info) {
  if (pmt_pid_info.stream_type != stream_type_e::iso_13818_pes_private)
    return false;

  auto buffer      = reinterpret_cast<uint8_t const *>(&pmt_descriptor + 1);
  auto num_entries = static_cast<unsigned int>(pmt_descriptor.length) / 5;
  type             = pid_type_e::subtitles;
  codec            = codec_c::look_up(codec_c::type_e::S_SRT);

  m_ttx_parser.reset(new teletext_to_srt_packet_converter_c);
  m_ttx_parser->parse_for_probing();

  mxdebug_if(reader.m_debug_pat_pmt, fmt::format("parse_teletext_pmt_descriptor: Teletext PMT descriptor, {0} entries\n", num_entries));

  for (auto idx = 0u; idx < num_entries; ++idx) {
    // EN 300 468, 6.2.43 "Teletext descriptor":

    // Bits:
    //  24: ISO 639 language code
    //   5: teletext type
    //   3: teletext magazine number
    //   8: teletext page number (4 bits: unit, 4 bits: tens)

    mtx::bits::reader_c r{buffer, 5};

    r.skip_bits(24);
    auto ttx_type     = r.get_bits(5);
    auto ttx_magazine = r.get_bits(3);
    auto ttx_page     = r.get_bits(4) * 10 + r.get_bits(4);
    ttx_page          = (ttx_magazine ? ttx_magazine : 8) * 100 + ttx_page;

    if (reader.m_debug_pat_pmt) {
      mxdebug(fmt::format(" {0}: language {1} type {2} magazine {3} page {4}\n",
                          idx, std::string(reinterpret_cast<char const *>(buffer), 3), ttx_type, ttx_magazine, ttx_page));
    }

    // Table 94: teletext_type
    //   0x00: reserved for future use
    //   0x01: initial Teletext page
    //   0x02: Teletext subtitles page
    //   0x03: additional information page
    //   0x04: program schedule page
    //   0x05: Teletext subtitle page for hearing impaired people
    if (!mtx::included_in(ttx_type, 2u, 5u)) {
      m_ttx_known_non_subtitle_pages[ttx_page] = true;
      buffer += 5;
      continue;
    }

    m_ttx_known_subtitle_pages[ttx_page] = true;

    auto new_track = set_up_teletext_track(ttx_page, ttx_type);
    auto set_up    = new_track ? new_track.get() : this;

    set_up->parse_iso639_language_from(buffer);

    buffer += 5;
  }

  return true;
}

track_ptr
track_c::set_up_teletext_track(int ttx_page,
                               int ttx_type) {
  track_c *to_set_up{};
  track_ptr to_return;

  if (!m_ttx_wanted_page) {
    converter.reset(new teletext_to_srt_packet_converter_c{});
    to_set_up = this;

  } else {
    to_return           = std::make_shared<track_c>(reader);
    to_return->m_master = this;
    m_coupled_tracks.emplace_back(to_return);

    to_set_up            = to_return.get();
    to_set_up->converter = converter;
    to_set_up->set_pid(pid);
  }

  to_set_up->m_ttx_wanted_page       = ttx_page;
  to_set_up->type                    = pid_type_e::subtitles;
  to_set_up->codec                   = codec_c::look_up(codec_c::type_e::S_SRT);
  to_set_up->m_hearing_impaired_flag = ttx_type == 5;
  to_set_up->probed_ok               = true;

  mxdebug_if(m_debug_headers, fmt::format("set_up_teletext_track: page {0} type {1} for {2}\n", ttx_page, ttx_type, to_set_up == this ? "this track" : "new track"));

  return to_return;
}

codec_c
track_c::determine_codec_for_hdmv_registration_descriptor(pmt_descriptor_t const &pmt_descriptor) {
  if (pmt_descriptor.length < 8)
    return {};

  // 187
  // Blu-ray specs, Table 9-9 – HDMV_video_registration_descriptor
  //   descriptor_tag     8
  //   descriptor_length  8
  //   format_identifier 32 ("HDMV")
  //   stuffing_bits      8 (0xff)
  //   stream_coding_type 8
  //   video_format       4
  //   frame_rate         4
  //   aspect_ratio       4
  //   stuffing_bits      4

  mtx::bits::reader_c r{reinterpret_cast<uint8_t const *>(&pmt_descriptor), static_cast<std::size_t>(pmt_descriptor.length + 2)};

  try {
    r.skip_bits(2 * 8 + 32);
    if (r.get_bits(8) != 0xff)
      return {};

    auto stream_coding_type = r.get_bits(8);
    auto derived_codec      = codec_c::look_up_bluray_stream_coding_type(stream_coding_type);
    auto video_format       = r.get_bits(4);
    auto frame_rate         = r.get_bits(4);
    auto aspect_ratio       = r.get_bits(4);

    if (reader.m_debug_pat_pmt) {
      auto frame_rate_desc    = frame_rate == 1 ? "24000/1001"
                              : frame_rate == 2 ? "24"
                              : frame_rate == 3 ? "25"
                              : frame_rate == 4 ? "30000/1001"
                              : frame_rate == 6 ? "50"
                              : frame_rate == 7 ? "60000/1001"
                              :                   "reserved";
      auto aspect_ratio_desc  = aspect_ratio == 2 ? "4:3"
                              : aspect_ratio == 3 ? "16:9"
                              :                     "reserved";

      mxdebug(fmt::format("determine_codec_for_hdmv_registration_descriptor: stream coding type: 0x{0:02x} ({1}) derived codec: {2} video format: {3} ({4}) frame rate: {5} ({6}) aspect ratio: {7} ({8})\n",
                          stream_coding_type, mtx::bluray::mpls::get_stream_coding_type_description(stream_coding_type), derived_codec,
                          video_format,       mtx::bluray::mpls::get_video_format_description(video_format),
                          frame_rate,         frame_rate_desc,
                          aspect_ratio,       aspect_ratio_desc));
    }

    return derived_codec;

  } catch (mtx::mm_io::end_of_file_x &) {
  }

  return {};
}

bool
track_c::parse_registration_pmt_descriptor(pmt_descriptor_t const &pmt_descriptor,
                                           pmt_pid_info_t const &pmt_pid_info) {
  if (reader.m_debug_pat_pmt) {
    mxdebug(fmt::format("parse_registration_pmt_descriptor: starting to parse the following:\n"));
    debugging_c::hexdump(reinterpret_cast<uint8_t const *>(&pmt_descriptor + 1), pmt_descriptor.length);
  }

  if (pmt_pid_info.stream_type != stream_type_e::iso_13818_pes_private)
    return false;

  if (pmt_descriptor.length < 4)
    return false;

  auto reg_fourcc = fourcc_c{reinterpret_cast<uint8_t const *>(&pmt_descriptor + 1)};
  auto reg_codec  = reg_fourcc == fourcc_c{"HDMV"} ? determine_codec_for_hdmv_registration_descriptor(pmt_descriptor) : codec_c::look_up(reg_fourcc.str());

  mxdebug_if(reader.m_debug_pat_pmt, fmt::format("parse_registration_pmt_descriptor: Registration descriptor with FourCC: {0} codec: {1}\n", reg_fourcc.description(), reg_codec));

  if (!reg_codec.valid())
    return false;

  switch (reg_codec.get_track_type()) {
    case track_audio:    type = pid_type_e::audio;     break;
    case track_video:    type = pid_type_e::video;     break;
    case track_subtitle: type = pid_type_e::subtitles; break;
    default:
      return false;
  }

  codec = reg_codec;

  return true;
}

bool
track_c::parse_subtitling_pmt_descriptor(pmt_descriptor_t const &pmt_descriptor,
                                         pmt_pid_info_t const &pmt_pid_info) {
  // Bits:
  // 24: ISO 639 language code
  //  8: subtitling type
  // 16: composition page ID
  // 16: ancillary page ID

  if (   (pmt_pid_info.stream_type != stream_type_e::iso_13818_pes_private)
      || (pmt_descriptor.length     < 8))
    return false;

  type                 = pid_type_e::subtitles;
  codec                = codec_c::look_up(codec_c::type_e::S_DVBSUB);
  m_codec_private_data = memory_c::alloc(5);
  auto codec_private   = m_codec_private_data->get_buffer();
  auto descriptor      = reinterpret_cast<uint8_t const *>(&pmt_descriptor + 1);

  parse_iso639_language_from(descriptor);

  put_uint16_be(&codec_private[0], get_uint16_be(&descriptor[4])); // composition page ID
  put_uint16_be(&codec_private[2], get_uint16_be(&descriptor[6])); // ancillary page ID
  codec_private[4] = descriptor[3];                                // subtitling type

  return true;
}

void
track_c::parse_iso639_language_from(void const *buffer) {
  auto value           = std::string{ reinterpret_cast<char const *>(buffer), 3 };
  auto parsed_language = mtx::bcp47::language_c::parse(value);

  if (parsed_language.has_valid_iso639_code())
    language = parsed_language;
}

std::size_t
track_c::remaining_payload_size_to_read()
  const {
  return pes_payload_size_to_read - std::min(pes_payload_size_to_read, pes_payload_read->get_size());
}

timestamp_c
track_c::derive_pts_from_content() {
  if (codec.is(codec_c::type_e::S_HDMV_TEXTST))
    return derive_hdmv_textst_pts_from_content();

  return {};
}

timestamp_c
track_c::derive_hdmv_textst_pts_from_content() {
  if (pes_payload_read->get_size() < 8)
    return {};

  auto &file = reader.m_files[m_file_num];
  auto buf   = pes_payload_read->get_buffer();

  if (static_cast<mtx::hdmv_textst::segment_type_e>(buf[0]) != mtx::hdmv_textst::dialog_presentation_segment)
    return file->m_timestamp_mpls_sync;

  auto stream_timestamp = timestamp_c::mpeg((static_cast<int64_t>(buf[3] & 1) << 32) | get_uint32_be(&buf[4]));
  auto diff             = file->m_timestamp_mpls_sync.value_or_zero() - reader.m_files[0]->m_timestamp_restriction_min.value_or_zero();
  auto timestamp        = stream_timestamp + diff;

  return timestamp;
}

void
track_c::determine_codec_from_stream_type(stream_type_e stream_type) {
  switch (stream_type) {
    case stream_type_e::iso_11172_video:
    case stream_type_e::iso_13818_video:
      type  = pid_type_e::video;
      codec = codec_c::look_up(codec_c::type_e::V_MPEG12);
      break;
    case stream_type_e::iso_14496_part2_video:
      type  = pid_type_e::video;
      codec = codec_c::look_up(codec_c::type_e::V_MPEG4_P2);
      break;
    case stream_type_e::iso_14496_part10_video:
      type  = pid_type_e::video;
      codec = codec_c::look_up(codec_c::type_e::V_MPEG4_P10);
      break;
    case stream_type_e::iso_23008_part2_video:
      type  = pid_type_e::video;
      codec = codec_c::look_up(codec_c::type_e::V_MPEGH_P2);
      break;
    case stream_type_e::stream_video_vc1:
      type  = pid_type_e::video;
      codec = codec_c::look_up(codec_c::type_e::V_VC1);
      break;
    case stream_type_e::iso_11172_audio:
    case stream_type_e::iso_13818_audio:
      type  = pid_type_e::audio;
      codec = codec_c::look_up(codec_c::type_e::A_MP3);
      break;
    case stream_type_e::iso_13818_part7_audio:
    case stream_type_e::iso_14496_part3_audio:
      type  = pid_type_e::audio;
      codec = codec_c::look_up(codec_c::type_e::A_AAC);
      break;
    case stream_type_e::stream_audio_pcm:
      type  = pid_type_e::audio;
      codec = codec_c::look_up(codec_c::type_e::A_PCM);
      break;

    case stream_type_e::stream_audio_ac3_lossless: {
      auto ac3_track       = std::make_shared<track_c>(*this);
      ac3_track->type      = pid_type_e::audio;
      ac3_track->codec     = codec_c::look_up(codec_c::type_e::A_AC3);
      ac3_track->converter = std::make_shared<truehd_ac3_splitting_packet_converter_c>();
      ac3_track->m_master  = this;
      ac3_track->set_pid(pid);

      type                 = pid_type_e::audio;
      codec                = codec_c::look_up(codec_c::type_e::A_TRUEHD);
      converter            = ac3_track->converter;
      m_coupled_tracks.push_back(ac3_track);

      break;
    }

    case stream_type_e::stream_audio_ac3:
    case stream_type_e::stream_audio_eac3:      // E-AC-3
    case stream_type_e::stream_audio_eac3_2:    // E-AC-3 secondary stream
    case stream_type_e::stream_audio_eac3_atsc: // E-AC-3 as defined in ATSC A/52:2012 Annex G
      type      = pid_type_e::audio;
      codec     = codec_c::look_up(codec_c::type_e::A_AC3);
      break;
    case stream_type_e::stream_audio_dts:
    case stream_type_e::stream_audio_dts_hd:
    case stream_type_e::stream_audio_dts_hd_ma:
    case stream_type_e::stream_audio_dts_hd2:
      type      = pid_type_e::audio;
      codec     = codec_c::look_up(codec_c::type_e::A_DTS);
      break;
    case stream_type_e::stream_subtitles_hdmv_pgs:
      type      = pid_type_e::subtitles;
      codec     = codec_c::look_up(codec_c::type_e::S_HDMV_PGS);
      probed_ok = true;
      break;
    case stream_type_e::stream_subtitles_hdmv_textst:
      type      = pid_type_e::subtitles;
      codec     = codec_c::look_up(codec_c::type_e::S_HDMV_TEXTST);
      break;
    case stream_type_e::iso_13818_pes_private:
      break;
    default:
      mxdebug_if(reader.m_debug_pat_pmt, fmt::format("parse_pmt: Unknown stream type: {0}\n", static_cast<int>(stream_type)));
      type      = pid_type_e::unknown;
      break;
  }
}

void
track_c::reset_processing_state() {
  m_timestamp.reset();
  m_previous_timestamp.reset();
  m_previous_valid_timestamp.reset();
  m_expected_next_continuity_counter.reset();

  clear_pes_payload();
  processed            = false;
  m_timestamps_wrapped = false;
  m_timestamp_wrap_add = timestamp_c::ns(0);
}

bool
track_c::transport_error_detected(packet_header_t &ts_header)
  const {
  if (ts_header.has_transport_error())
    return true;

  if (!m_expected_next_continuity_counter)
    return false;

  return ts_header.continuity_counter() != m_expected_next_continuity_counter.value();
}

void
track_c::set_packetizer_source_id()
  const {
  if (!reader.m_is_reading_mpls || (ptzr < 0))
    return;

  reader.m_reader_packetizers[ptzr]->set_source_id(fmt::format("00{0:04x}", pid));
}

bool
track_c::contains_dovi_base_layer_for_enhancement_layer(track_c const &el_track)
  const {
  if (&el_track == this)
    return false;

  if (el_track.codec != codec)
    return false;

  if (!el_track.m_dovi_config)
    return false;

  auto profile         = el_track.m_dovi_config->dv_profile;
  auto resolution_type = (v_width == 3840) && (v_height == 2160) ? 'U'
                       : (v_width == 1920) && (v_height == 1080) ? 'F'
                       :                                           '?';

  if (!mtx::included_in(profile, 4, 7))
    return false;

  if (profile == 4) {
    if ((resolution_type == 'F') && (el_track.v_width == (v_width / 2)) && (el_track.v_height == (v_height / 2)))
      return true;

  } else {                      // profile == 7
    if ((resolution_type == 'F') && (el_track.v_width == v_width) && (el_track.v_height == v_height))
      return true;

    else if ((resolution_type == 'U') && (el_track.v_width == (v_width / 2)) && (el_track.v_height == (v_height / 2)))
      return true;
  }

  return false;
}

void
track_c::handle_probed_teletext_pages(std::vector<track_ptr> &newly_created_tracks) {
  if (!m_ttx_parser)
    return;

  std::vector<int> pages;

  if (m_debug_headers) {
    for (auto const &itr : m_ttx_known_subtitle_pages)
      pages.push_back(itr.first);
    std::sort(pages.begin(), pages.end());

    mxdebug(fmt::format("handle_probed_teletext_pages: known subtitle pages: <{0}>\n", mtx::string::join(pages, ",")));

    pages.clear();

    for (auto const &itr : m_ttx_known_non_subtitle_pages)
      pages.push_back(itr.first);
    std::sort(pages.begin(), pages.end());

    mxdebug(fmt::format("handle_probed_teletext_pages: known non-subtitle pages: <{0}>\n", mtx::string::join(pages, ",")));
  }

  pages = m_ttx_parser->get_probed_subtitle_page_numbers();
  std::sort(pages.begin(), pages.end());

  mxdebug_if(m_debug_headers, fmt::format("handle_probed_teletext_pages: probed pages: <{0}>\n", mtx::string::join(pages, ",")));

  for (auto const &page : pages) {
    if (m_ttx_known_subtitle_pages[page] || m_ttx_known_non_subtitle_pages[page])
      continue;

    mxdebug_if(m_debug_headers, fmt::format("handle_probed_teletext_pages:   adding track for page {0}\n", page));

    auto new_track = set_up_teletext_track(page);
    if (new_track)
      newly_created_tracks.push_back(new_track);
  }
}

// ------------------------------------------------------------

file_t::file_t(mm_io_cptr const &in)
  : m_in{in}
  , m_pat_found{}
  , m_num_pmts_found{}
  , m_num_pmts_to_find{}
  , m_es_to_process{}
  , m_stream_timestamp{timestamp_c::ns(0)}
  , m_timestamp_mpls_sync{timestamp_c::ns(0)}
  , m_state{processing_state_e::probing}
  , m_probe_range{}
  , m_file_done{}
  , m_packet_sent_to_packetizer{}
  , m_detected_packet_size{}
  , m_num_pat_crc_errors{}
  , m_num_pmt_crc_errors{}
  , m_validate_pat_crc{true}
  , m_validate_pmt_crc{true}
  , m_has_audio_or_video_track{}
{
}

int64_t
file_t::get_queued_bytes()
  const {
  int64_t bytes{};

  for (auto ptzr : m_packetizers)
    bytes += ptzr->get_queued_bytes();

  return bytes;
}

void
file_t::reset_processing_state(processing_state_e new_state) {
  m_state = new_state;
  m_last_non_subtitle_pts.reset();
  m_last_non_subtitle_dts.reset();
}

bool
file_t::all_pmts_found()
  const {
  return (0 != m_num_pmts_to_find) && (m_num_pmts_found >= m_num_pmts_to_find);
}

uint64_t
file_t::get_start_source_packet_position()
  const {
  return m_start_source_packet_number * m_detected_packet_size;
}

// ------------------------------------------------------------

bool
reader_c::probe_file() {
  return detect_packet_size(*m_in, m_in->get_size()).has_value();
}

std::optional<std::pair<unsigned int, unsigned int>>
reader_c::detect_packet_size(mm_io_c &in,
                             uint64_t size) {
  debugging_option_c debug{"mpeg_ts|mpeg_ts_packet_size"};

  try {
    size                         = std::min<uint64_t>(1024 * 1024, size);
    auto num_startcodes_required = std::max<uint64_t>(size / 3 / TS_MAX_PACKET_SIZE, 32);
    auto buffer                  = memory_c::alloc(size);
    auto mem                     = buffer->get_buffer();

    mxdebug_if(debug, fmt::format("detect_packet_size: size to probe {0} num required startcodes {1}\n", size, num_startcodes_required));

    in.setFilePointer(0);
    size = in.read(mem, size);

    std::vector<int> positions;
    for (int i = 0; i < static_cast<int>(size); ++i)
      if (0x47 == mem[i])
        positions.push_back(i);

    for (int i = 0, num_positions = positions.size(); i < num_positions; ++i) {
      for (int k = 0; 0 != potential_packet_sizes[k]; ++k) {
        unsigned int pos            = positions[i];
        unsigned int packet_size    = potential_packet_sizes[k];
        unsigned int num_startcodes = 1;

        while ((num_startcodes_required > num_startcodes) && (pos < size) && (0x47 == mem[pos])) {
          pos += packet_size;
          ++num_startcodes;
        }

        if (num_startcodes_required <= num_startcodes) {
          auto offset = 0u;

          // Blu-ray specs 6.2.1: 192 byte "source packets" that start
          // with a four-byte "TP_extra_header" followed by 188 bytes
          // actual "transport packet". The latter one is the one
          // starting with 0x47.
          if (   (packet_size  == 192)
              && (positions[i] ==   4))
            offset = 4;

          mxdebug_if(debug, fmt::format("detect_packet_size: detected packet size {0} at offset {1}; returning offset {2}\n", packet_size, positions[i], offset));

          return std::make_pair(packet_size, offset);
        }
      }
    }
  } catch (...) {
  }

  mxdebug_if(debug, "detect_packet_size: packet size could not be determined\n");

  return {};
}

void
reader_c::setup_initial_tracks() {
  m_tracks.clear();

  m_tracks.push_back(std::make_shared<track_c>(*this, pid_type_e::pat));
  m_tracks.push_back(std::make_shared<track_c>(*this, pid_type_e::sdt));
  m_tracks.back()->set_pid(TS_SDT_PID);

  auto &f                      = file();
  f.m_ignored_pids[TS_PAT_PID] = true;
  f.m_ignored_pids[TS_SDT_PID] = true;
}

void
reader_c::read_headers_for_file(std::size_t file_num) {
  m_current_file = file_num;
  auto &f        = file();

  setup_initial_tracks();

  try {
    auto file_size           = f.m_in->get_size();
    m_bytes_to_process      += file_size;
    f.m_probe_range          = calculate_probe_range(file_size, 10 * 1024 * 1024);
    auto size_to_probe       = std::min<uint64_t>(file_size,     f.m_probe_range);
    auto min_size_to_probe   = std::min<uint64_t>(size_to_probe, 5 * 1024 * 1024);
    auto detection_result    = detect_packet_size(*f.m_in, size_to_probe);
    f.m_detected_packet_size = detection_result->first;
    f.m_header_offset        = detection_result->second;

    f.m_in->setFilePointer(0);

    mxdebug_if(m_debug_headers, fmt::format("read_headers: Starting to build PID list. (packet size: {0})\n", f.m_detected_packet_size));

    uint8_t buf[TS_MAX_PACKET_SIZE]; // maximum TS packet size + 1

    while (true) {
      if (f.m_in->read(buf, f.m_detected_packet_size) != static_cast<unsigned int>(f.m_detected_packet_size))
        break;

      if (buf[f.m_header_offset] != 0x47) {
        if (resync(f.m_in->getFilePointer() - f.m_detected_packet_size))
          continue;
        break;
      }

      parse_packet(&buf[f.m_header_offset]);

      if (   f.m_pat_found
          && f.all_pmts_found()
          && (0 == f.m_es_to_process)
          && (f.m_in->getFilePointer() >= min_size_to_probe))
        break;

      auto eof = f.m_in->eof() || (f.m_in->getFilePointer() >= size_to_probe);
      if (!eof)
        continue;

      // Determine if we haven't found a PAT or a PMT but have plenty
      // of CRC errors (e.g. for badly mastered discs). In such a case
      // we should read from the start again, this time ignoring the
      // errors for the specific type.
      mxdebug_if(m_debug_headers,
                 fmt::format("read_headers: EOF during detection. #tracks {0} #PAT CRC errors {1} #PMT CRC errors {2} PAT found {3} PMT found {4}/{5}\n",
                             m_tracks.size(), f.m_num_pat_crc_errors, f.m_num_pmt_crc_errors, f.m_pat_found, f.m_num_pmts_found, f.m_num_pmts_to_find));

      if (!f.m_pat_found && f.m_validate_pat_crc)
        f.m_validate_pat_crc = false;

      else if (f.m_pat_found && !f.all_pmts_found() && f.m_validate_pmt_crc) {
        f.m_validate_pmt_crc = false;
        f.m_num_pmts_to_find = 0;
        f.m_pmt_pid_seen.clear();

      } else
        break;

      f.m_in->setFilePointer(0);
      f.m_in->clear_eof();

      setup_initial_tracks();
    }
  } catch (...) {
    mxdebug_if(m_debug_headers, fmt::format("read_headers: caught exception\n"));
  }

  mxdebug_if(m_debug_headers, fmt::format("read_headers: Detection done on {0} bytes\n", f.m_in->getFilePointer()));

  // Run probe_packet_complete() for track-type detection once for
  // each track. This way tracks that don't actually need their
  // content to be found during probing will be set to OK, too.
  for (auto const &track : m_tracks) {
    if (track->is_pes_payload_size_unbounded() && (track->pes_payload_read->get_size() >= 6))
      parse_pes(*track);

    track->clear_pes_payload();
    probe_packet_complete(*track, true);
  }

  pair_dovi_base_and_enhancement_layer_tracks();
  handle_probed_teletext_pages();

  std::copy(m_tracks.begin(), m_tracks.end(), std::back_inserter(m_all_probed_tracks));
}

void
reader_c::pair_dovi_base_and_enhancement_layer_tracks() {
  if (mtx::hacks::is_engaged(mtx::hacks::KEEP_DOLBY_VISION_LAYERS_SEPARATE))
    return;

  for (std::size_t idx = 0, num_tracks = m_tracks.size(); idx < num_tracks; ++idx) {
    auto &track = *m_tracks[idx];

    if (track.m_hidden || !track.m_dovi_config)
      continue;

    mxdebug_if(m_debug_dovi, fmt::format("pair_dovi_base_and_enhancement_layer_tracks: PID {0}: looking for a suitable base layer track; base layer PID from PMT: {1}\n",
                                         track.pid, track.m_dovi_base_layer_pid ? fmt::to_string(*track.m_dovi_base_layer_pid) : "–"s));

    track_c *bl_track = nullptr;

    if (track.m_dovi_base_layer_pid) {
      auto itr = std::find_if(m_tracks.begin(), m_tracks.end(), [&track](auto const &track_candidate) {
        return (track_candidate.get() != &track) && (track_candidate->pid == *track.m_dovi_base_layer_pid);
      });

      if (itr != m_tracks.end())
        bl_track = itr->get();

    } else {
      for (std::size_t candidate_idx = 0; candidate_idx < num_tracks; ++candidate_idx) {
        if (m_tracks[candidate_idx]->contains_dovi_base_layer_for_enhancement_layer(track)) {
          bl_track = m_tracks[candidate_idx].get();
          break;
        }
      }
    }

    if (!bl_track) {
      mxdebug_if(m_debug_dovi, fmt::format("pair_dovi_base_and_enhancement_layer_tracks: PID {0}: no suitable track found; ignoring track\n", track.pid));
      continue;
    }

    mxdebug_if(m_debug_dovi, fmt::format("pair_dovi_base_and_enhancement_layer_tracks: PID {0}: found base layer track with PID {1}\n", track.pid, bl_track->pid));

    track.m_hidden            = true;
    bl_track->m_dovi_el_track = &track;
  }
}

void
reader_c::handle_probed_teletext_pages() {
  std::vector<track_ptr> newly_created_tracks;

  for (auto const &track : m_tracks)
    track->handle_probed_teletext_pages(newly_created_tracks);

  std::copy(newly_created_tracks.begin(), newly_created_tracks.end(), std::back_inserter(m_tracks));
}

void
reader_c::determine_start_source_packet_number(file_t &file) {
  mxdebug_if(m_debug_mpls, fmt::format("MPLS: start SPN: file {0} min timestamp restriction {1}\n", to_utf8(Q(file.m_in->get_file_name()).replace(QRegularExpression{".*[/\\\\]"}, {})), file.m_timestamp_restriction_min));

  if (!file.m_clpi_parser || !file.m_timestamp_restriction_min.valid()) {
    mxdebug_if(m_debug_mpls, fmt::format("MPLS: start SPN: clip information not found or no minimum timestamp restriction\n"));
    return;
  }

  std::vector<uint64_t> source_packet_numbers;

  for (auto const &ep_map : file.m_clpi_parser->m_ep_map) {
    std::optional<uint64_t> source_packet_number;

    for (auto const &point : ep_map.points) {
      if (point.pts <= file.m_timestamp_restriction_min)
        source_packet_number = point.spn;
      else
        break;
    }

    if (source_packet_number) {
      mxdebug_if(m_debug_mpls, fmt::format("MPLS: start SPN: candidate SPN for PID {0} is {1}\n", ep_map.pid, *source_packet_number));
      source_packet_numbers.push_back(*source_packet_number);
    }
  }

  if (!source_packet_numbers.empty())
    file.m_start_source_packet_number = *std::min_element(source_packet_numbers.begin(), source_packet_numbers.end());

  mxdebug_if(m_debug_mpls, fmt::format("MPLS: start SPN: chosen start SPN is {0}\n", file.m_start_source_packet_number));
}

void
reader_c::read_headers() {
  m_files.emplace_back(std::make_shared<file_t>(m_in));

  m_files[0]->m_timestamp_restriction_min = m_restricted_timestamps_min;
  m_files[0]->m_timestamp_restriction_max = m_restricted_timestamps_max;

  auto mpls_in = dynamic_cast<mm_mpls_multi_file_io_c *>(get_underlying_input());
  if (mpls_in) {
    m_is_reading_mpls = true;
    m_mpls_chapters   = mpls_in->get_chapters();

    add_external_files_from_mpls(*mpls_in);
  }

  for (int idx = 0, num_files = m_files.size(); idx < num_files; ++idx)
    read_headers_for_file(idx);

  m_tracks = std::move(m_all_probed_tracks);

  for (int idx = 0, num_files = m_files.size(); idx < num_files; ++idx) {
    parse_clip_info_file(idx);
    determine_start_source_packet_number(*m_files[idx]);
  }

  process_chapter_entries();

  std::vector<track_ptr> identified_tracks;

  auto track_id = uint64_t{};
  for (auto &track : m_tracks) {
    // For TrueHD tracks detection for embedded AC-3 frames is
    // done. However, "probed_ok" is only set on the TrueHD track if
    // both types have been found. If only TrueHD is found then
    // "probed_ok" must be set to true after detection has exhausted
    // the search space; otherwise a TrueHD-only track would never be
    // considered OK.
    if (track->codec.is(codec_c::type_e::A_TRUEHD) && track->m_truehd_found_truehd)
      track->probed_ok = true;

    if (!track->probed_ok || !track->codec)
      continue;

    track->clear_pes_payload();
    track->processed        = false;
    track->m_id             = track_id++;
    // track->timestamp_offset = -1;

    for (auto const &coupled_track : track->m_coupled_tracks)
      if (!coupled_track->language.is_valid())
        coupled_track->language = track->language;

    identified_tracks.push_back(track);
  }

  m_tracks = std::move(identified_tracks);

  show_demuxer_info();
}

void
reader_c::reset_processing_state(processing_state_e new_state) {
  for (auto const &file : m_files)
    file->reset_processing_state(new_state);

  for (auto const &track : m_tracks)
    track->reset_processing_state();

  m_packet_num = 0;
}

void
reader_c::determine_global_timestamp_offset() {
  reset_processing_state(processing_state_e::determining_timestamp_offset);

  auto &f = file();

  auto probe_start_pos = f.get_start_source_packet_position();
  auto probe_end_pos   = probe_start_pos + f.m_probe_range;

  f.m_in->setFilePointer(probe_start_pos);
  f.m_in->clear_eof();

  mxdebug_if(m_debug_timestamp_offset, fmt::format("determine_global_timestamp_offset: determining global timestamp offset from the first {0} bytes\n", f.m_probe_range));

  try {
    uint8_t buf[TS_MAX_PACKET_SIZE]; // maximum TS packet size + 1

    while (f.m_in->getFilePointer() < probe_end_pos) {
      if (f.m_in->read(buf, f.m_detected_packet_size) != static_cast<unsigned int>(f.m_detected_packet_size))
        break;

      if (buf[f.m_header_offset] != 0x47) {
        if (resync(f.m_in->getFilePointer() - f.m_detected_packet_size))
          continue;
        break;
      }

      parse_packet(&buf[f.m_header_offset]);
    }
  } catch (...) {
    mxdebug_if(m_debug_timestamp_offset, fmt::format("determine_global_timestamp_offset: caught exception\n"));
  }

  mxdebug_if(m_debug_timestamp_offset, fmt::format("determine_global_timestamp_offset: detection done; global timestamp offset is {0}\n", f.m_global_timestamp_offset));

  f.m_in->setFilePointer(f.get_start_source_packet_position());
  f.m_in->clear_eof();

  reset_processing_state(processing_state_e::muxing);

  if (debugging_c::requested("mpeg_ts_dont_offset_timestamps"))
    for (auto const &file : m_files)
      file->m_global_timestamp_offset = timestamp_c::ns(0);
}

void
reader_c::process_chapter_entries() {
  if (m_mpls_chapters.empty() || m_ti.m_no_chapters)
    return;

  auto language    = m_ti.m_chapter_language.is_valid() ? m_ti.m_chapter_language
                   : g_default_language.is_valid()      ? g_default_language
                   :                                      mtx::bcp47::language_c::parse("eng");

  m_chapters       = mtx::chapters::convert_mpls_chapters_kax_chapters(m_mpls_chapters, language);

  auto const &sync = mtx::includes(m_ti.m_timestamp_syncs, track_info_c::chapter_track_id) ? m_ti.m_timestamp_syncs[track_info_c::chapter_track_id]
                   : mtx::includes(m_ti.m_timestamp_syncs, track_info_c::all_tracks_id)    ? m_ti.m_timestamp_syncs[track_info_c::all_tracks_id]
                   :                                                                         timestamp_sync_t{};
  mtx::chapters::adjust_timestamps(*m_chapters, sync.displacement, sync.factor);
}

uint32_t
reader_c::calculate_crc(void const *buffer,
                        size_t size)
  const {
  return mtx::bytes::swap_32(mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::crc32_ieee, buffer, size, 0xffffffff));
}

void
reader_c::add_multiplexed_ids(std::vector<uint64_t> &multiplexed_ids,
                              track_c &track) {
  multiplexed_ids.push_back(track.m_id);
  for (auto const &coupled_track : track.m_coupled_tracks)
    if (coupled_track->probed_ok)
      multiplexed_ids.push_back(coupled_track->m_id);
}

void
reader_c::add_programs_to_identification_info(mtx::id::info_c &info) {
  if (m_files[0]->m_programs.empty())
    return;

  std::sort(m_files[0]->m_programs.begin(), m_files[0]->m_programs.end());

  auto programs_json = nlohmann::json::array();

  for (auto &program : m_files[0]->m_programs) {
    auto program_json = nlohmann::json{
      { mtx::id::program_number,   program.program_number },
      { mtx::id::service_provider, program.service_provider },
      { mtx::id::service_name,     program.service_name },
    };

    programs_json.push_back(program_json);
  }

  info.add(mtx::id::programs, programs_json);
}

void
reader_c::identify() {
  auto info    = mtx::id::info_c{};
  auto mpls_in = dynamic_cast<mm_mpls_multi_file_io_c *>(get_underlying_input());
  if (mpls_in)
    mpls_in->create_verbose_identification_info(info);

  add_programs_to_identification_info(info);

  id_result_container(info.get());

  for (auto const &track : m_tracks) {
    if (track->m_hidden)
      continue;

    info = mtx::id::info_c{};
    info.add(mtx::id::language,       track->language.get_iso639_alpha_3_code());
    info.set(mtx::id::stream_id,      track->pid);
    info.set(mtx::id::number,         track->pid);
    info.add(mtx::id::program_number, track->program_number);

    if (pid_type_e::audio == track->type) {
      info.add(mtx::id::audio_channels,           track->a_channels);
      info.add(mtx::id::audio_sampling_frequency, track->a_sample_rate);
      info.add(mtx::id::audio_bits_per_sample,    track->a_bits_per_sample);

    } else if (pid_type_e::video == track->type)
      info.add_joined(mtx::id::pixel_dimensions, "x"s, track->v_width, track->v_height);

    else if (pid_type_e::subtitles == track->type) {
      info.set(mtx::id::text_subtitles,        track->codec.is(codec_c::type_e::S_SRT));
      info.add(mtx::id::teletext_page,         track->m_ttx_wanted_page);
      info.add(mtx::id::flag_hearing_impaired, track->m_hearing_impaired_flag);
    }

    auto multiplexed_track_ids = std::vector<uint64_t>{};
    if (!track->m_coupled_tracks.empty())
      add_multiplexed_ids(multiplexed_track_ids, *track);

    else if (track->m_master)
      add_multiplexed_ids(multiplexed_track_ids, *track->m_master);

    if (!multiplexed_track_ids.empty()) {
      std::sort(multiplexed_track_ids.begin(), multiplexed_track_ids.end());
      info.set(mtx::id::multiplexed_tracks, multiplexed_track_ids);
    }

    std::string type = pid_type_e::audio == track->type ? ID_RESULT_TRACK_AUDIO
                     : pid_type_e::video == track->type ? ID_RESULT_TRACK_VIDEO
                     :                                    ID_RESULT_TRACK_SUBTITLES;

    id_result_track(track->m_id, type, track->codec.get_name(), info.get());
  }

  if (!m_mpls_chapters.empty())
    id_result_chapters(m_mpls_chapters.size());
}

bool
reader_c::parse_pat(track_c &track) {
  if (track.pes_payload_read->get_size() < sizeof(pat_t)) {
    mxdebug_if(m_debug_pat_pmt, "Invalid parameters!\n");
    return false;
  }

  auto pat        = track.pes_payload_read->get_buffer();
  auto pat_header = reinterpret_cast<pat_t *>(pat);

  if (pat_header->table_id != 0x00) {
    mxdebug_if(m_debug_pat_pmt, "Invalid PAT table_id!\n");
    return false;
  }

  if (pat_header->get_section_syntax_indicator() != 1 || pat_header->get_current_next_indicator() == 0) {
    mxdebug_if(m_debug_pat_pmt, "Invalid PAT section_syntax_indicator/current_next_indicator!\n");
    return false;
  }

  if (pat_header->section_number != 0 || pat_header->last_section_number != 0) {
    mxdebug_if(m_debug_pat_pmt, "Unsupported multiple section PAT!\n");
    return false;
  }

  auto &f                           = file();
  unsigned short pat_section_length = pat_header->get_section_length();
  uint32_t elapsed_CRC              = calculate_crc(pat, 3 + pat_section_length - 4/*CRC32*/);
  uint32_t read_CRC                 = get_uint32_be(pat + 3 + pat_section_length - 4);

  if (elapsed_CRC != read_CRC) {
    mxdebug_if(m_debug_pat_pmt, fmt::format("parse_pat: Wrong PAT CRC !!! Elapsed = 0x{0:08x}, read 0x{1:08x}, validate PAT CRC? {2}\n", elapsed_CRC, read_CRC, f.m_validate_pat_crc));
    ++f.m_num_pat_crc_errors;
    if (f.m_validate_pat_crc)
      return false;
  }

  if (pat_section_length < 13 || pat_section_length > 1021) {
    mxdebug_if(m_debug_pat_pmt, fmt::format("parse_pat: Wrong PAT section_length (= {0})\n", pat_section_length));
    return false;
  }

  auto prog_count  = static_cast<unsigned int>(pat_section_length - 5 - 4/*CRC32*/) / 4;
  auto pat_section = reinterpret_cast<pat_section_t *>(pat + sizeof(pat_t));

  for (auto i = 0u; i < prog_count; i++, pat_section++) {
    unsigned short local_program_number = pat_section->get_program_number();
    uint16_t tmp_pid                    = pat_section->get_pid();

    mxdebug_if(m_debug_pat_pmt,
               fmt::format("parse_pat: program_number: {0}; {1}_pid: {2}\n",
                           local_program_number,
                           0 == local_program_number ? "nit" : "pmt",
                           tmp_pid));

    if (!local_program_number || f.m_pmt_pid_seen[tmp_pid])
      continue;

    auto pmt                  = std::make_shared<track_c>(*this, pid_type_e::pmt);
    f.m_es_to_process         = 0;
    f.m_ignored_pids[tmp_pid] = true;
    f.m_pmt_pid_seen[tmp_pid] = true;
    f.m_pat_found             = true;
    ++f.m_num_pmts_to_find;

    pmt->set_pid(tmp_pid);

    m_tracks.push_back(pmt);
  }

  mxdebug_if(m_debug_pat_pmt, fmt::format("parse_pat: number of PMTs to find: {0}\n", f.m_num_pmts_to_find));

  return true;
}

memory_cptr
reader_c::read_pmt_descriptor(mm_io_c &io) {
  auto buffer = io.read(sizeof(pmt_descriptor_t));
  auto length = buffer->get_buffer()[1];

  buffer->resize(sizeof(pmt_descriptor_t) + length);
  if (io.read(buffer->get_buffer() + sizeof(pmt_descriptor_t), length) != length)
    throw mtx::mm_io::end_of_file_x{};

  return buffer;
}

bool
reader_c::parse_pmt_pid_info(mm_mem_io_c &mem,
                             uint16_t program_number) {
  auto &f                  = file();
  auto missing_tag         = true;

  auto pmt_pid_info_buffer = mem.read(sizeof(pmt_pid_info_t));
  auto pmt_pid_info        = reinterpret_cast<pmt_pid_info_t *>(pmt_pid_info_buffer->get_buffer());
  auto es_info_length      = pmt_pid_info->get_es_info_length();

  auto track               = std::make_shared<track_c>(*this);
  track->type              = pid_type_e::unknown;
  track->program_number    = program_number;

  track->set_pid(pmt_pid_info->get_pid());
  track->determine_codec_from_stream_type(pmt_pid_info->stream_type);

  while (es_info_length >= sizeof(pmt_descriptor_t)) {
    auto pmt_descriptor_buffer = read_pmt_descriptor(mem);
    auto pmt_descriptor        = reinterpret_cast<pmt_descriptor_t *>(pmt_descriptor_buffer->get_buffer());

    mxdebug_if(m_debug_pat_pmt, fmt::format("parse_pmt: PID {2} PMT descriptor tag 0x{0:02x} length {1}\n", static_cast<unsigned int>(pmt_descriptor->tag), static_cast<unsigned int>(pmt_descriptor->length), track->pid));

    if (pmt_descriptor_buffer->get_size() > es_info_length)
      break;

    es_info_length -= pmt_descriptor_buffer->get_size();

    if (0x0a != pmt_descriptor->tag)
      missing_tag = false;

    switch (pmt_descriptor->tag) {
      case 0x05: // registration descriptor
        track->parse_registration_pmt_descriptor(*pmt_descriptor, *pmt_pid_info);
        break;
      case 0x0a: // ISO 639 language descriptor
        track->parse_iso639_language_from(pmt_descriptor + 1);
        break;
      case 0x56: // Teletext descriptor
        track->parse_teletext_pmt_descriptor(*pmt_descriptor, *pmt_pid_info);
        break;
      case 0x59: // Subtitles descriptor
        track->parse_subtitling_pmt_descriptor(*pmt_descriptor, *pmt_pid_info);
        break;
      case 0x6A: // AC-3 descriptor
      case 0x7A: // E-AC-3 descriptor
        track->parse_ac3_pmt_descriptor(*pmt_descriptor, *pmt_pid_info);
        break;
      case 0x7b: // DTS descriptor
        track->parse_dts_pmt_descriptor(*pmt_descriptor, *pmt_pid_info);
        break;
      case 0xb0: // Dolby Vision descriptor
        track->parse_dovi_pmt_descriptor(*pmt_descriptor, *pmt_pid_info);
        break;
    }
  }

  // Default to AC-3 if it's a PES private stream type that's missing
  // a known/more concrete descriptor tag.
  if ((pmt_pid_info->stream_type == stream_type_e::iso_13818_pes_private) && missing_tag) {
    track->type  = pid_type_e::audio;
    track->codec = codec_c::look_up(codec_c::type_e::A_AC3);
  }

  if (track->type != pid_type_e::unknown) {
    track->processed = false;
    m_tracks.push_back(track);
    ++f.m_es_to_process;

    std::copy(track->m_coupled_tracks.begin(), track->m_coupled_tracks.end(), std::back_inserter(m_tracks));
    f.m_es_to_process += track->m_coupled_tracks.size();
  }

  mxdebug_if(m_debug_pat_pmt,
             fmt::format("parse_pmt: PID {0} stream type {1:02x} has type: {2}\n",
                         track->pid,
                         static_cast<unsigned int>(pmt_pid_info->stream_type),
                         track->type != pid_type_e::unknown ? track->codec.get_name() : "<unknown>"));

  return true;
}

bool
reader_c::parse_pmt(track_c &track) {
  auto payload_size = track.pes_payload_read->get_size();
  auto pmt_start    = track.pes_payload_read->get_buffer();
  auto pmt_end      = pmt_start + payload_size;

  if (payload_size < sizeof(pmt_t)) {
    mxdebug_if(m_debug_pat_pmt, fmt::format("Actual PMT size {0} less than PMT structure size {1}!\n", payload_size, sizeof(pmt_t)));
    return false;
  }

  auto pmt_header     = reinterpret_cast<pmt_t *>(pmt_start);
  auto program_number = get_uint16_be(&pmt_header->program_number);

  if (pmt_header->table_id != 0x02) {
    mxdebug_if(m_debug_pat_pmt, "Invalid PMT table_id!\n");
    return false;
  }

  if (pmt_header->get_section_syntax_indicator() != 1 || pmt_header->get_current_next_indicator() == 0) {
    mxdebug_if(m_debug_pat_pmt, "Invalid PMT section_syntax_indicator/current_next_indicator!\n");
    return false;
  }

  if (pmt_header->section_number != 0 || pmt_header->last_section_number != 0) {
    mxdebug_if(m_debug_pat_pmt, "Unsupported multiple section PMT!\n");
    return false;
  }

  auto &f                  = file();
  auto pmt_section_length  = pmt_header->get_section_length();
  auto pmt_section_start   = pmt_start         + offsetof(pmt_t, program_number);
  auto pmt_section_end     = pmt_section_start + pmt_section_length;
  auto program_info_length = pmt_header->get_program_info_length();
  auto program_info_start  = pmt_start + sizeof(pmt_t);
  auto program_info_end    = program_info_start + program_info_length;

  if (program_info_end > pmt_end) {
    mxdebug_if(m_debug_pat_pmt, fmt::format("Actual PMT size {0} less than PMT program info size + program info offset {1}!\n", payload_size, pmt_end - pmt_start));
    return false;
  }

  if (pmt_section_end > pmt_end) {
    mxdebug_if(m_debug_pat_pmt, fmt::format("Actual PMT size {0} less than PMT section size + section offset {1}!\n", payload_size, pmt_end - pmt_start));
    return false;
  }

  if ((pmt_section_length < 13) || (pmt_section_length > 1021)) {
    mxdebug_if(m_debug_pat_pmt, fmt::format("parse_pmt: Wrong PMT section_length {0}\n", pmt_section_length));
    return false;
  }

  uint32_t elapsed_CRC = calculate_crc(pmt_start, 3 + pmt_section_length - 4/*CRC32*/);
  uint32_t read_CRC    = get_uint32_be(pmt_section_end - 4);

  if (elapsed_CRC != read_CRC) {
    mxdebug_if(m_debug_pat_pmt, fmt::format("parse_pmt: Wrong PMT CRC !!! Elapsed = 0x{0:08x}, read 0x{1:08x}, validate PMT CRC? {2}\n", elapsed_CRC, read_CRC, f.m_validate_pmt_crc));
    ++f.m_num_pmt_crc_errors;
    if (f.m_validate_pmt_crc)
      return false;
  }

  mxdebug_if(m_debug_pat_pmt,
             fmt::format("parse_pmt: program number: {0} PCR PID: {1} program info length: {2}\n",
                         pmt_header->get_program_number(), pmt_header->get_pcr_pid(), program_info_length));

  mm_mem_io_c mem{program_info_end, static_cast<uint64_t>(pmt_section_end - 4 - program_info_end)};

  try {
    bool ok = true;
    while (ok)
      ok = parse_pmt_pid_info(mem, program_number);

  } catch (mtx::mm_io::exception &) {
  }

  f.m_ignored_pids[track.pid] = true;

  ++f.m_num_pmts_found;

  mxdebug_if(m_debug_pat_pmt,
             fmt::format("parse_pmt: {0} num PMTs found {1} vs. to find {2}\n",
                           f.m_num_pmts_found  < f.m_num_pmts_to_find ? "find_ongoing"
                         : f.m_num_pmts_found == f.m_num_pmts_to_find ? "find_done"
                         :                                              "find_error",
                         f.m_num_pmts_found, f.m_num_pmts_to_find));

  return true;
}

charset_converter_cptr
reader_c::get_charset_converter_for_coding_type(unsigned int coding) {
  static std::unordered_map<unsigned int, std::string> coding_names;

  if (coding_names.empty()) {
    coding_names[0x00]     = "ISO6937";
    coding_names[0x01]     = "ISO8859-5";
    coding_names[0x02]     = "ISO8859-6";
    coding_names[0x03]     = "ISO8859-7";
    coding_names[0x04]     = "ISO8859-8";
    coding_names[0x05]     = "ISO8859-9";
    coding_names[0x06]     = "ISO8859-10";
    coding_names[0x07]     = "ISO8859-11";
    coding_names[0x09]     = "ISO8859-13";
    coding_names[0x0a]     = "ISO8859-14";
    coding_names[0x0b]     = "ISO8859-15";
    coding_names[0x10]     = "ISO8859";
    coding_names[0x13]     = "GB2312";
    coding_names[0x14]     = "BIG5";
    coding_names[0x100001] = "ISO8859-1";
    coding_names[0x100002] = "ISO8859-2";
    coding_names[0x100003] = "ISO8859-3";
    coding_names[0x100004] = "ISO8859-4";
    coding_names[0x100005] = "ISO8859-5";
    coding_names[0x100006] = "ISO8859-6";
    coding_names[0x100007] = "ISO8859-7";
    coding_names[0x100008] = "ISO8859-8";
    coding_names[0x100009] = "ISO8859-9";
    coding_names[0x10000a] = "ISO8859-10";
    coding_names[0x10000b] = "ISO8859-11";
    coding_names[0x10000d] = "ISO8859-13";
    coding_names[0x10000e] = "ISO8859-14";
    coding_names[0x10000f] = "ISO8859-15";
  }

  auto coding_name = coding_names[coding];
  if (coding_name.empty())
    coding_name = "UTF-8";

  auto converter = charset_converter_c::init(coding_name, true);
  return converter ? converter : charset_converter_c::init("UTF-8");
}

std::string
reader_c::read_descriptor_string(mtx::bits::reader_c &r) {
  auto str_length = r.get_bits(8);
  if (!str_length)
    return {};

  std::string content(str_length, ' ');

  r.get_bytes(reinterpret_cast<uint8_t *>(&content[0]), str_length);

  auto coding = static_cast<unsigned int>(content[0]);

  if (coding >= 0x20)
    coding = 0x00;

  else if (coding == 0x10) {
    if (str_length < 4)
      return {};

    coding = 0x100000 | get_uint16_be(&content[1]);
    content.erase(0, 3);

  } else if (coding == 0x1f) {
    if (str_length < 3)
      return {};

    coding = 0x1f00 | static_cast<uint8_t>(content[1]);
    content.erase(0, 2);

  } else
    content.erase(0, 1);

  return get_charset_converter_for_coding_type(coding)->utf8(content);
}

void
reader_c::parse_sdt_service_desciptor(mtx::bits::reader_c &r,
                                      uint16_t program_number) {
  r.skip_bits(8);               // service_type

  auto service_provider = read_descriptor_string(r);
  auto service_name     = read_descriptor_string(r);

  mxdebug_if(m_debug_sdt, fmt::format("parse_sdt: program_number {0} service_provider {1} service_name {2}\n", program_number, service_provider, service_name));

  file().m_programs.push_back({ program_number, service_provider, service_name });
}

bool
reader_c::parse_sdt(track_c &track) {
  if (!track.pes_payload_read->get_size())
    return false;

  at_scope_exit_c finally{[&track]() { track.clear_pes_payload(); }};

  try {
    mtx::bits::reader_c r{track.pes_payload_read->get_buffer(), track.pes_payload_read->get_size()};

    if (r.get_bits(8) != TS_SDT_TID)
      return false;

    r.skip_bits(1 + 1 + 2       // section_syntax_indicator, reserved_for_future_use, reserved
                + 12 + 16       // section_length, transport_stream_id
                + 2 + 5 + 1     // reserved, version_number, current_next_indicator
                + 8 + 8         // section_number, last_section_number
                + 16 + 8);      // original_network_id, reserved_future_use

    while (r.get_remaining_bits() > 32) { // only CRC_32
      auto program_number = r.get_bits(16);

      r.skip_bits(6 + 1 + 1     // reserved_future_use, EIT_schedule_flag, EIT_Present_following_flag
                  + 3 + 1);     // running_status, free_CA_mode

      auto descriptors_loop_end_bits = r.get_bit_position() + 12 + r.get_bits(12) * 8;

      while ((r.get_bit_position() + 16u) < descriptors_loop_end_bits) {
        auto start   = r.get_bit_position();
        auto tag     = r.get_bits(8);
        auto length  = r.get_bits(8);

        if ((start + 8 * (2 + length)) > descriptors_loop_end_bits)
          break;

        if (tag == 0x48)
          parse_sdt_service_desciptor(r, program_number);

        r.set_bit_position(start + 8 * (2 + length));
      }
    }

  } catch (mtx::mm_io::exception &) {
    mxdebug_if(m_debug_sdt, fmt::format("parse_sdt: exception during SDT parsing\n"));
    return false;
  }

  return true;
}

void
reader_c::parse_pes(track_c &track) {
  at_scope_exit_c finally{[&track]() { track.clear_pes_payload(); }};

  auto &f             = file();
  auto pes            = track.pes_payload_read->get_buffer();
  auto const pes_size = track.pes_payload_read->get_size();
  auto pes_header     = reinterpret_cast<pes_header_t *>(pes);

  if (pes_size < 6) {
    mxdebug_if(m_debug_packet, fmt::format("parse_pes: error: PES payload ({0}) too small even for the basic PES header (6)\n", pes_size));
    return;
  }

  timestamp_c pts, dts;
  auto has_pts = false;
  auto has_dts = false;
  auto to_skip = 0u;

  if (mtx::included_in(pes_header->stream_id, mtx::mpeg1_2::PRIVATE_STREAM_2)) {
    to_skip = 6;
    track.pes_payload_read->remove(to_skip);

    pts = track.derive_pts_from_content();
    dts = pts;

  } else {
    if (pes_size < sizeof(pes_header_t)) {
      mxdebug_if(m_debug_packet, fmt::format("parse_pes: error: PES payload ({0}) too small for PES header structure itself ({1})\n", pes_size, sizeof(pes_header_t)));
      return;
    }

    has_pts = (pes_header->get_pts_dts_flags() & 0x02) == 0x02; // 10 and 11 mean PTS is present
    has_dts = (pes_header->get_pts_dts_flags() & 0x01) == 0x01; // 01 and 11 mean DTS is present
    to_skip = offsetof(pes_header_t, pes_header_data_length) + pes_header->pes_header_data_length + 1;

    if (pes_size < to_skip) {
      mxdebug_if(m_debug_packet, fmt::format("parse_pes: error: PES payload ({0}) too small for PES header + header data including PTS/DTS ({1})\n", pes_size, to_skip));
      return;
    }

    if (has_pts) {
      pts = read_timestamp(&pes_header->pts_dts);
      dts = pts;
    }

    if (has_dts)
      dts = read_timestamp(&pes_header->pts_dts + (has_pts ? 5 : 0));

    track.pes_payload_read->remove(to_skip);
  }

  if (!track.m_use_dts)
    dts = pts;

  auto orig_pts = pts;
  auto orig_dts = dts;

  auto result = track.handle_bogus_subtitle_timestamps(pts, dts);

  if (drop_decision_e::drop == result)
    return;

  track.handle_timestamp_wrap(pts, dts);

  auto set_global_timestamp_offset_from_pts
     = pts.valid()
    && (mtx::included_in(track.type, pid_type_e::audio, pid_type_e::video))
    && (!f.m_global_timestamp_offset.valid()   || (pts <  f.m_global_timestamp_offset))
    && (!f.m_timestamp_restriction_min.valid() || (pts >= f.m_timestamp_restriction_min));

  if (set_global_timestamp_offset_from_pts) {
    mxdebug_if(m_debug_headers,
               fmt::format("determining_timestamp_offset: new global timestamp offset {0} prior {1} file position afterwards {2} min_restriction {3} DTS {4}\n",
                           pts, f.m_global_timestamp_offset, f.m_in->getFilePointer(), f.m_timestamp_restriction_min, dts));
    f.m_global_timestamp_offset = pts;
  }

  if (processing_state_e::determining_timestamp_offset == f.m_state)
    return;

  if (f.m_timestamp_restriction_max.valid() && has_pts && (pts >= f.m_timestamp_restriction_max)) {
    mxdebug_if(m_debug_mpls, fmt::format("MPLS: stopping processing file as PTS {0} >= max. timestamp restriction {1}\n", pts, f.m_timestamp_restriction_max));
    f.m_in->setFilePointer(f.m_in->get_size());
    return;
  }

  if (m_debug_packet) {
    mxdebug(fmt::format("parse_pes: PES info at file position {0} (file num {1}):\n", f.m_position, track.m_file_num));
    mxdebug(fmt::format("parse_pes:    stream_id = {0} PID = {1}\n", static_cast<unsigned int>(pes_header->stream_id), track.pid));
    mxdebug(fmt::format("parse_pes:    PES_packet_length = {0}, PES_header_data_length = {1}, data starts at {2}\n", pes_size, static_cast<unsigned int>(pes_header->pes_header_data_length), to_skip));
    mxdebug(fmt::format("parse_pes:    PTS? {0} ({4} processed {5}) DTS? ({6} processed {7}) {1} ESCR = {2} ES_rate = {3}\n",
                        has_pts, has_dts, static_cast<unsigned int>(pes_header->get_escr()), static_cast<unsigned int>(pes_header->get_es_rate()), orig_pts, pts, orig_dts, dts));
    mxdebug(fmt::format("parse_pes:    DSM_trick_mode = {0}, add_copy = {1}, CRC = {2}, ext = {3}\n",
                        static_cast<unsigned int>(pes_header->get_dsm_trick_mode()), static_cast<unsigned int>(pes_header->get_additional_copy_info()), static_cast<unsigned int>(pes_header->get_pes_crc()),
                        static_cast<unsigned int>(pes_header->get_pes_extension())));
  }

  if (pts.valid()) {
    track.m_previous_valid_timestamp = pts;
    f.m_stream_timestamp             = pts;
    track.m_timestamp                = dts;

    mxdebug_if(m_debug_packet, fmt::format("parse_pes: PID {0} PTS found: {1} DTS: {2}\n", track.pid, pts, dts));
  }

  if (processing_state_e::probing == f.m_state)
    probe_packet_complete(track);

  else
    track.send_to_packetizer();
}

timestamp_c
reader_c::read_timestamp(uint8_t *p) {
  int64_t mpeg_timestamp  =  static_cast<int64_t>(             ( p[0]   >> 1) & 0x07) << 30;
  mpeg_timestamp         |= (static_cast<int64_t>(get_uint16_be(&p[1])) >> 1)         << 15;
  mpeg_timestamp         |=  static_cast<int64_t>(get_uint16_be(&p[3]))               >>  1;

  return timestamp_c::mpeg(mpeg_timestamp);
}

track_ptr
reader_c::handle_packet_for_pid_not_listed_in_pmt(uint16_t pid) {
  auto &f = file();

  if (   (f.m_state != processing_state_e::probing)
      || f.m_ignored_pids[pid]
      || !f.m_pat_found
      || !f.all_pmts_found())
    return {};

  mxdebug_if(m_debug_pat_pmt, fmt::format("found packet for track PID {0} not listed in PMT, attempting type detection by content\n", pid));

  auto track = std::make_shared<track_c>(*this);
  track->set_pid(pid);

  m_tracks.push_back(track);
  ++f.m_es_to_process;

  return track;
}

void
reader_c::parse_packet(uint8_t *buf) {
  auto hdr   = reinterpret_cast<packet_header_t *>(buf);
  auto track = find_track_for_pid(hdr->get_pid());

  if (!track)
    track = handle_packet_for_pid_not_listed_in_pmt(hdr->get_pid());

  if (   !track
      || !hdr->has_payload())   // no ts_payload
    return;

  if (mtx::included_in(track->type, pid_type_e::video, pid_type_e::audio, pid_type_e::subtitles, pid_type_e::unknown))
    handle_transport_errors(*track, *hdr);

  if (   hdr->has_transport_error() // corrupted packet
      || track->processed)
    return;

  auto payload = determine_ts_payload_start(hdr);

  if (payload.second)
    handle_ts_payload(*track, *hdr, payload.first, payload.second);
}

void
reader_c::handle_ts_payload(track_c &track,
                            packet_header_t &ts_header,
                            uint8_t *ts_payload,
                            std::size_t ts_payload_size) {
  if (mtx::included_in(track.type, pid_type_e::pat, pid_type_e::pmt, pid_type_e::sdt))
    handle_pat_pmt_payload(track, ts_header, ts_payload, ts_payload_size);

  else if (mtx::included_in(track.type, pid_type_e::video, pid_type_e::audio, pid_type_e::subtitles, pid_type_e::unknown))
    handle_pes_payload(track, ts_header, ts_payload, ts_payload_size);

  else {
    mxdebug_if(m_debug_headers, fmt::format("handle_ts_payload: error: unknown track.type {0}\n", static_cast<unsigned int>(track.type)));
  }
}

void
reader_c::handle_pat_pmt_payload(track_c &track,
                                 packet_header_t &ts_header,
                                 uint8_t *ts_payload,
                                 std::size_t ts_payload_size) {
  auto &f = file();

  if (processing_state_e::probing != f.m_state)
    return;

  if (ts_header.is_payload_unit_start()) {
    auto to_skip = static_cast<std::size_t>(ts_payload[0]) + 1;

    if (to_skip >= ts_payload_size) {
      mxdebug_if(m_debug_headers, fmt::format("handle_pat_pmt_payload: error: TS payload size {0} too small; needed to skip {1}\n", ts_payload_size, to_skip));
      track.pes_payload_size_to_read = 0;
      return;
    }

    track.clear_pes_payload();
    auto pat_table                   = reinterpret_cast<pat_t *>(ts_payload + to_skip);
    ts_payload_size                 -= to_skip;
    ts_payload                      += to_skip;
    track.pes_payload_size_to_read  = pat_table->get_section_length() + 3;
  }

  track.add_pes_payload(ts_payload, ts_payload_size);

  if (track.is_pes_payload_complete())
    probe_packet_complete(track);
}

drop_decision_e
reader_c::handle_transport_errors(track_c &track,
                                  packet_header_t &ts_header) {
  auto decision = drop_decision_e::keep;

  if (track.transport_error_detected(ts_header) && !track.m_skip_pes_payload) {
    mxdebug_if(m_debug_pes_headers,
               fmt::format("handle_transport_errors: error detected for track PID {0} (0x{0:04x}) at {1} packet num {7} ({2}, expected continuity_counter {3} actual {4}); dropping current PES packet (expected size {5} read {6})\n",
                           track.pid,
                           file().m_position,
                           ts_header.has_transport_error() ? "transport_error flag" : "wrong continuity_counter",
                           track.m_expected_next_continuity_counter ? fmt::to_string(static_cast<unsigned int>(track.m_expected_next_continuity_counter.value())) : "—"s,
                           static_cast<unsigned int>(ts_header.continuity_counter()),
                           track.pes_payload_size_to_read,
                           track.pes_payload_read->get_size(),
                           m_packet_num));

    track.clear_pes_payload();
    track.m_skip_pes_payload = true;
    decision                 = drop_decision_e::drop;
  }

  track.m_expected_next_continuity_counter = (ts_header.continuity_counter() + (ts_header.has_payload() ? 1 : 0)) % 16;

  return decision;
}

void
reader_c::handle_pes_payload(track_c &track,
                             packet_header_t &ts_header,
                             uint8_t *ts_payload,
                             std::size_t ts_payload_size) {
  if (ts_header.is_payload_unit_start()) {
    if (track.is_pes_payload_size_unbounded() && track.pes_payload_read->get_size())
      parse_pes(track);

    if (track.pes_payload_size_to_read && track.pes_payload_read->get_size() && m_debug_pes_headers)
      mxdebug(fmt::format("handle_pes_payload: error: dropping incomplete PES payload; target size {0} already read {1}\n", track.pes_payload_size_to_read, track.pes_payload_read->get_size()));

    track.clear_pes_payload();

    if (ts_payload_size < sizeof(pes_header_t)) {
      mxdebug_if(m_debug_pes_headers, fmt::format("handle_pes_payload: error: TS payload size {0} too small for PES header {1}\n", ts_payload_size, sizeof(pes_header_t)));
      track.m_skip_pes_payload = true;
      return;
    }

    auto const start_code = get_uint24_be(ts_payload);
    if (start_code != 0x000001) {
      mxdebug_if(m_debug_pes_headers, fmt::format("handle_pes_payload: error: PES header in TS payload does not start with proper start code; actual: {0:06x}\n", start_code));
      track.m_skip_pes_payload = true;
      return;
    }

    auto pes_header                = reinterpret_cast<pes_header_t *>(ts_payload);
    track.pes_payload_size_to_read = pes_header->get_pes_packet_length() + offsetof(pes_header_t, flags1);
    track.m_skip_pes_payload       = false;
  }

  if (track.m_skip_pes_payload)
    return;

  track.add_pes_payload(ts_payload, ts_payload_size);

  if (track.is_pes_payload_complete())
    parse_pes(track);
}

void
reader_c::determine_track_type_by_pes_content(track_c &track) {
  auto buffer = track.pes_payload_read->get_buffer();
  auto size   = track.pes_payload_read->get_size();

  if (!size)
    return;

  if (m_debug_pat_pmt) {
    auto data = mtx::string::to_hex(buffer, std::min<unsigned int>(size, 16));
    mxdebug(fmt::format("determine_track_type_by_pes_content: PID {0}: attempting type detection from content for with size: {1} have DOVI config: {2} first 16 bytes: {3}\n", track.pid, size, track.m_dovi_config.has_value(), data));
  }

  if ((size >= 3) && ((get_uint24_be(buffer) & mtx::aac::ADTS_SYNC_WORD_MASK) == mtx::aac::ADTS_SYNC_WORD)) {
    track.type  = pid_type_e::audio;
    track.codec = codec_c::look_up(codec_c::type_e::A_AAC);

  } else if ((size >= 2) && (get_uint16_be(buffer) == mtx::ac3::SYNC_WORD)) {
    track.type  = pid_type_e::audio;
    track.codec = codec_c::look_up(codec_c::type_e::A_AC3);
  }

  mxdebug_if(m_debug_pat_pmt, fmt::format("determine_track_type_by_pes_content: detected type: {0} codec: {1}\n", static_cast<unsigned int>(track.type), track.codec));
}

int
reader_c::determine_track_parameters(track_c &track,
                                     bool end_of_detection) {
  if (track.type == pid_type_e::pat)
    return parse_pat(track) ? 0 : -1;

  else if (track.type == pid_type_e::pmt)
    return parse_pmt(track) ? 0 : -1;

  else if (track.type == pid_type_e::sdt)
    return parse_sdt(track) ? 0 : -1;

  else if (track.type == pid_type_e::unknown)
    determine_track_type_by_pes_content(track);

  if (track.type == pid_type_e::video) {
    if (track.codec.is(codec_c::type_e::V_MPEG12))
      return track.new_stream_v_mpeg_1_2(end_of_detection);
    else if (track.codec.is(codec_c::type_e::V_MPEG4_P10))
      return track.new_stream_v_avc(end_of_detection);
    else if (track.codec.is(codec_c::type_e::V_MPEGH_P2))
      return track.new_stream_v_hevc(end_of_detection);
    else if (track.codec.is(codec_c::type_e::V_VC1))
      return track.new_stream_v_vc1();

    track.pes_payload_read->set_chunk_size(512 * 1024);

  } else if (track.type == pid_type_e::subtitles) {
    if (track.codec.is(codec_c::type_e::S_HDMV_TEXTST))
      return track.new_stream_s_hdmv_textst();

    else if (track.codec.is(codec_c::type_e::S_DVBSUB))
      return track.new_stream_s_dvbsub();

    else if (track.codec.is(codec_c::type_e::S_SRT))
      return track.new_stream_s_teletext();

  } else if (track.type != pid_type_e::audio)
    return -1;

  if (track.codec.is(codec_c::type_e::A_MP3))
    return track.new_stream_a_mpeg();
  else if (track.codec.is(codec_c::type_e::A_AAC))
    return track.new_stream_a_aac();
  else if (track.codec.is(codec_c::type_e::A_AC3))
    return track.new_stream_a_ac3();
  else if (track.codec.is(codec_c::type_e::A_DTS))
    return track.new_stream_a_dts();
  else if (track.codec.is(codec_c::type_e::A_PCM))
    return track.new_stream_a_pcm();
  else if (track.codec.is(codec_c::type_e::A_TRUEHD))
    return track.new_stream_a_truehd();

  return -1;
}

void
reader_c::probe_packet_complete(track_c &track,
                                bool end_of_detection) {
  if (track.probed_ok) {
    track.clear_pes_payload();
    return;
  }

  int result = -1;

  try {
    result = determine_track_parameters(track, end_of_detection);
  } catch (...) {
  }

  track.clear_pes_payload();

  if (result != 0) {
    mxdebug_if(m_debug_headers, (result == FILE_STATUS_MOREDATA) ? "probe_packet_complete: Need more data to probe ES\n" : "probe_packet_complete: Failed to parse packet. Reset and retry\n");
    track.processed = false;

    return;
  }

  if (mtx::included_in(track.type, pid_type_e::pat, pid_type_e::pmt)) {
    auto it = std::find_if(m_tracks.begin(), m_tracks.end(), [&track](track_ptr const &candidate) { return candidate.get() == &track; });
    if (m_tracks.end() != it)
      m_tracks.erase(it);

  } else {
    auto &f         = file();
    track.processed = true;
    track.probed_ok = true;
    --f.m_es_to_process;

    if (mtx::included_in(track.type, pid_type_e::audio, pid_type_e::video))
      f.m_has_audio_or_video_track = true;

    mxdebug_if(m_debug_headers, fmt::format("probe_packet_complete: ES to process: {0}\n", f.m_es_to_process));
  }
}

void
reader_c::create_packetizer(int64_t id) {
  if ((0 > id) || (m_tracks.size() <= static_cast<size_t>(id)))
    return;

  auto &track = m_tracks[id];
  auto type   = track->type == pid_type_e::audio ? 'a'
              : track->type == pid_type_e::video ? 'v'
              :                                    's';

  if (!track->probed_ok || track->m_hidden || (0 == track->ptzr) || !demuxing_requested(type, id, track->language))
    return;

  m_ti.m_id       = id;
  m_ti.m_language = track->language;

  if (pid_type_e::audio == track->type) {
    if (track->codec.is(codec_c::type_e::A_MP3))
      track->ptzr = add_packetizer(new mp3_packetizer_c(this, m_ti, track->a_sample_rate, track->a_channels, (0 != track->a_sample_rate) && (0 != track->a_channels)));

    else if (track->codec.is(codec_c::type_e::A_AAC))
      create_aac_audio_packetizer(track);

    else if (track->codec.is(codec_c::type_e::A_AC3))
      create_ac3_audio_packetizer(track);

    else if (track->codec.is(codec_c::type_e::A_DTS))
      track->ptzr = add_packetizer(new dts_packetizer_c(this, m_ti, track->a_dts_header));

    else if (track->codec.is(codec_c::type_e::A_PCM))
      create_pcm_audio_packetizer(track);

    else if (track->codec.is(codec_c::type_e::A_TRUEHD))
      create_truehd_audio_packetizer(track);

  } else if (pid_type_e::video == track->type) {
    if (track->codec.is(codec_c::type_e::V_MPEG12))
      create_mpeg1_2_video_packetizer(track);

    else if (track->codec.is(codec_c::type_e::V_MPEG4_P10))
      create_mpeg4_p10_es_video_packetizer(track);

    else if (track->codec.is(codec_c::type_e::V_MPEGH_P2))
      create_mpegh_p2_es_video_packetizer(track);

    else if (track->codec.is(codec_c::type_e::V_VC1))
      create_vc1_video_packetizer(track);

  } else if (track->codec.is(codec_c::type_e::S_HDMV_PGS))
    create_hdmv_pgs_subtitles_packetizer(track);

  else if (track->codec.is(codec_c::type_e::S_HDMV_TEXTST))
    create_hdmv_textst_subtitles_packetizer(track);

  else if (track->codec.is(codec_c::type_e::S_SRT))
    create_srt_subtitles_packetizer(track);

  else if (track->codec.is(codec_c::type_e::S_DVBSUB))
    create_dvbsub_subtitles_packetizer(track);

  if (-1 == track->ptzr)
    return;

  auto &packetizer                 = ptzr(track->ptzr);
  m_ptzr_to_track_map[&packetizer] = track;

  m_files[track->m_file_num]->m_packetizers.push_back(&packetizer);

  track->set_packetizer_source_id();

  if (track->m_hearing_impaired_flag.has_value() &&track->m_hearing_impaired_flag.value())
    packetizer.set_hearing_impaired_flag(true, option_source_e::container);

  show_packetizer_info(id, packetizer);
}

void
reader_c::create_aac_audio_packetizer(track_ptr const &track) {
  auto aac_packetizer = new aac_packetizer_c(this, m_ti, track->m_aac_frame.m_header.config, aac_packetizer_c::headerless);
  track->ptzr         = add_packetizer(aac_packetizer);
  track->converter    = std::make_shared<aac_framing_packet_converter_c>(&ptzr(track->ptzr), track->m_aac_multiplex_type);

  if (mtx::aac::PROFILE_SBR == track->m_aac_frame.m_header.config.profile)
    aac_packetizer->set_audio_output_sampling_freq(track->m_aac_frame.m_header.config.sample_rate * 2);
}

void
reader_c::create_ac3_audio_packetizer(track_ptr const &track) {
  track->ptzr = add_packetizer(new ac3_packetizer_c(this, m_ti, track->a_sample_rate, track->a_channels, track->a_bsid));

  if (track->converter)
    dynamic_cast<truehd_ac3_splitting_packet_converter_c &>(*track->converter).set_ac3_packetizer(&ptzr(track->ptzr));
}

void
reader_c::create_pcm_audio_packetizer(track_ptr const &track) {
  track->ptzr = add_packetizer(new pcm_packetizer_c(this, m_ti, track->a_sample_rate, track->a_channels, track->a_bits_per_sample, pcm_packetizer_c::big_endian_integer));

  if ((track->a_sample_rate == 0) || !mtx::included_in(track->a_bits_per_sample, 16, 24))
    return;

  track->converter = std::make_shared<bluray_pcm_channel_layout_packet_converter_c>(track->a_bits_per_sample / 8, track->a_channels + 1, track->a_channels);
  track->converter->set_packetizer(&ptzr(track->ptzr));
}

void
reader_c::create_truehd_audio_packetizer(track_ptr const &track) {
  track->ptzr = add_packetizer(new truehd_packetizer_c(this, m_ti, mtx::truehd::frame_t::truehd, track->a_sample_rate, track->a_channels));

  if (track->converter)
    dynamic_cast<truehd_ac3_splitting_packet_converter_c &>(*track->converter).set_packetizer(&ptzr(track->ptzr));
}

void
reader_c::create_mpeg1_2_video_packetizer(track_ptr &track) {
  m_ti.m_private_data = track->m_codec_private_data;
  auto m2vpacketizer  = new mpeg1_2_video_packetizer_c(this, m_ti, track->v_version, mtx::to_int(track->m_default_duration), track->v_width, track->v_height, track->v_dwidth, track->v_dheight, false);
  track->ptzr         = add_packetizer(m2vpacketizer);

  m2vpacketizer->set_video_interlaced_flag(track->v_interlaced);
}

void
reader_c::create_mpeg4_p10_es_video_packetizer(track_ptr &track) {
  track->ptzr = add_packetizer(new avc_es_video_packetizer_c(this, m_ti, track->v_width, track->v_height));
}

void
reader_c::create_mpegh_p2_es_video_packetizer(track_ptr &track) {
  track->ptzr = add_packetizer(new hevc_es_video_packetizer_c(this, m_ti, track->v_width, track->v_height));

  if (!track->m_dovi_el_track)
    return;

  track->converter                  = std::make_shared<hevc_dovi_layer_combiner_packet_converter_c>(static_cast<hevc_es_video_packetizer_c *>(&ptzr(track->ptzr)), track->pid, track->m_dovi_el_track->pid);
  track->m_dovi_el_track->converter = track->converter;
}

void
reader_c::create_vc1_video_packetizer(track_ptr &track) {
  track->m_use_dts = true;
  track->ptzr      = add_packetizer(new vc1_video_packetizer_c(this, m_ti));
}

void
reader_c::create_hdmv_pgs_subtitles_packetizer(track_ptr &track) {
  auto ptzr = new hdmv_pgs_packetizer_c(this, m_ti);
  ptzr->set_aggregate_packets(true);
  track->ptzr = add_packetizer(ptzr);
}

void
reader_c::create_hdmv_textst_subtitles_packetizer(track_ptr const &track) {
  track->ptzr = add_packetizer(new hdmv_textst_packetizer_c(this, m_ti, track->m_codec_private_data));
}

void
reader_c::create_srt_subtitles_packetizer(track_ptr const &track) {
  auto recoding_requested = mtx::includes(m_ti.m_sub_charsets, track->m_id) || mtx::includes(m_ti.m_sub_charsets, track->m_id);
  track->ptzr             = add_packetizer(new textsubs_packetizer_c(this, m_ti, MKV_S_TEXTUTF8, recoding_requested));

  auto &converter         = dynamic_cast<teletext_to_srt_packet_converter_c &>(*track->converter);

  converter.demux_page(*track->m_ttx_wanted_page, &ptzr(track->ptzr));
  converter.override_encoding(*track->m_ttx_wanted_page, track->language.get_iso639_alpha_3_code());
}

void
reader_c::create_dvbsub_subtitles_packetizer(track_ptr const &track) {
  track->ptzr = add_packetizer(new dvbsub_packetizer_c{this, m_ti, track->m_codec_private_data});
  track->converter.reset(new dvbsub_pes_framing_removal_packet_converter_c{&ptzr(track->ptzr)});
}

void
reader_c::create_packetizers() {
  determine_global_timestamp_offset();

  mxdebug_if(m_debug_headers, fmt::format("create_packetizers: create packetizers...\n"));
  for (int i = 0, end = m_tracks.size(); i < end; ++i)
    create_packetizer(i);
}

void
reader_c::add_available_track_ids() {
  add_available_track_id_range(0, m_tracks.size() - 1);

  if (m_chapters)
    add_available_track_id(track_info_c::chapter_track_id);
}

bool
reader_c::all_files_done()
  const {
  for (auto const &file : m_files)
    if (!file->m_file_done)
      return false;

  return true;
}

file_status_e
reader_c::finish() {
  if (all_files_done()) {
    m_bytes_processed = m_bytes_to_process;
    return flush_packetizers();
  }

  for (auto &track : m_tracks) {
    if (track->m_file_num != m_current_file)
      continue;

    if ((-1 != track->ptzr) && (0 < track->pes_payload_read->get_size()))
      parse_pes(*track);

    if (track->converter)
      track->converter->flush();

    if (-1 != track->ptzr)
      ptzr(track->ptzr).flush();
  }

  file().m_file_done = true;

  return FILE_STATUS_DONE;
}

file_status_e
reader_c::read(generic_packetizer_c *requested_ptzr,
               bool force) {
  if (all_files_done())
    return flush_packetizers();

  auto &requested_ptzr_track = m_ptzr_to_track_map[requested_ptzr];
  if (!requested_ptzr_track)
    return flush_packetizers();

  m_current_file        = requested_ptzr_track->m_file_num;
  auto &f               = file();
  auto num_queued_bytes = f.get_queued_bytes();

  if (!force && (20 * 1024 * 1024 < num_queued_bytes)) {
    if (   !requested_ptzr_track
        || !mtx::included_in(requested_ptzr_track->type, pid_type_e::audio, pid_type_e::video)
        || (512 * 1024 * 1024 < num_queued_bytes))
      return FILE_STATUS_HOLDING;
  }

  f.m_packet_sent_to_packetizer = false;
  auto prior_position           = f.m_in->getFilePointer();

  uint8_t buf[TS_MAX_PACKET_SIZE + 1];

  while (!f.m_packet_sent_to_packetizer) {
    f.m_position = f.m_in->getFilePointer();

    if (f.m_in->read(buf, f.m_detected_packet_size) != static_cast<unsigned int>(f.m_detected_packet_size))
      return finish();

    if (buf[f.m_header_offset] != 0x47) {
      if (resync(f.m_position))
        continue;
      return finish();
    }

    ++m_packet_num;

    parse_packet(&buf[f.m_header_offset]);
  }

  m_bytes_processed += f.m_in->getFilePointer() - prior_position;

  return FILE_STATUS_MOREDATA;
}

void
reader_c::parse_clip_info_file(std::size_t file_idx) {
  auto &file         = *m_files[file_idx];
  auto mpls_multi_in = dynamic_cast<mm_mpls_multi_file_io_c *>(get_underlying_input(file.m_in.get()));
  auto source_file   = mpls_multi_in ? mpls_multi_in->get_file_names()[0] : file.m_in->get_file_name();

  mxdebug_if(m_debug_clpi, fmt::format("find_clip_info_file: Searching for CLPI corresponding to {0}\n", source_file.string()));

  auto clpi_file = mtx::bluray::find_other_file(source_file, mtx::fs::to_path("CLIPINF") / mtx::fs::to_path(fmt::format("{0}.clpi", source_file.stem().string())));

  mxdebug_if(m_debug_clpi, fmt::format("reader_c::find_clip_info_file: CLPI file: {0}\n", !clpi_file.empty() ? clpi_file.string() : "not found"));

  if (clpi_file.empty())
    return;

  file.m_clpi_parser.reset(new mtx::bluray::clpi::parser_c{clpi_file.string()});
  if (!file.m_clpi_parser->parse()) {
    file.m_clpi_parser.reset();
    return;
  }

  for (auto &track : m_tracks) {
    if (track->m_file_num != file_idx)
      continue;

    bool found = false;

    for (auto &program : file.m_clpi_parser->m_programs) {
      for (auto &stream : program->program_streams) {
        if ((stream->pid != track->pid) || !stream->language.is_valid())
          continue;

        track->language = stream->language;
        found = true;
        break;
      }

      if (found)
        break;
    }
  }
}

bool
reader_c::resync(int64_t start_at) {
  auto &f = file();

  try {
    mxdebug_if(m_debug_resync, fmt::format("resync: Start resync for data from {0}\n", start_at));
    f.m_in->setFilePointer(start_at);

    uint8_t buf[TS_MAX_PACKET_SIZE + 1];

    while (!f.m_in->eof()) {
      auto curr_pos = f.m_in->getFilePointer();
      buf[0]        = f.m_in->read_uint8();

      if (0x47 != buf[0])
        continue;

      if (f.m_in->read(&buf[1], f.m_detected_packet_size) != static_cast<unsigned int>(f.m_detected_packet_size))
        return false;

      if (0x47 != buf[f.m_detected_packet_size]) {
        f.m_in->setFilePointer(curr_pos + 1);
        continue;
      }

      if (curr_pos >= f.m_header_offset)
        curr_pos -= f.m_header_offset;

      mxdebug_if(m_debug_resync, fmt::format("resync: Re-established at {0}\n", curr_pos));

      f.m_in->setFilePointer(curr_pos);
      return true;
    }

  } catch (...) {
    return false;
  }

  return false;
}

track_ptr
reader_c::find_track_for_pid(uint16_t pid)
  const {
  auto &f = *m_files[m_current_file];

  for (auto const &track : m_tracks) {
    if (   (track->m_file_num != m_current_file)
        || (track->pid        != pid))
      continue;

    if (track->has_packetizer() || mtx::included_in(f.m_state, processing_state_e::probing, processing_state_e::determining_timestamp_offset))
      return track;

    for (auto const &coupled_track : track->m_coupled_tracks)
      if (coupled_track->has_packetizer())
        return coupled_track;

    return track;
  }

  return {};
}

std::pair<uint8_t *, std::size_t>
reader_c::determine_ts_payload_start(packet_header_t *hdr)
  const {
  auto ts_header_start  = reinterpret_cast<uint8_t *>(hdr);
  auto ts_payload_start = ts_header_start + sizeof(packet_header_t);

  if (hdr->has_adaptation_field())
    // Adaptation field's first byte is its length - 1.
    ts_payload_start += ts_payload_start[0] + 1;

  // Make sure not to overflow the TS packet buffer.
  ts_payload_start = std::min(ts_payload_start, ts_header_start + TS_PACKET_SIZE);

  return { ts_payload_start, static_cast<std::size_t>(ts_header_start + TS_PACKET_SIZE - ts_payload_start) };
}

file_t &
reader_c::file() {
  return *m_files[m_current_file];
}

void
reader_c::add_external_files_from_mpls(mm_mpls_multi_file_io_c &mpls_in) {
  auto source_file  = mpls_in.get_file_names()[0];
  auto parser       = mpls_in.get_mpls_parser();
  auto &playlist    = parser.get_playlist();
  auto sub_path_idx = 0u;

  if (!playlist.items.empty()) {
    auto &item                        = playlist.items.front();
    auto &file                        = m_files.front();

    file->m_timestamp_restriction_min = item.in_time;
    file->m_timestamp_restriction_max = item.out_time;

    mxdebug_if(m_debug_mpls, fmt::format("add_external_files_from_mpls: main restrictions for {0} min {1} max {2}\n", mpls_in.get_file_name(), item.in_time, item.out_time));
  }

  if (playlist.sub_paths.empty())
    return;

  for (auto const &sub_path : playlist.sub_paths) {
    ++sub_path_idx;

    if ((mtx::bluray::mpls::sub_path_type_e::text_subtitle_presentation != sub_path.type) || sub_path.items.empty())
      continue;

    auto &item = sub_path.items.front();
    auto m2ts  = mtx::bluray::find_other_file(source_file, mtx::fs::to_path("STREAM") / mtx::fs::to_path(fmt::format("{0}.m2ts", mtx::fs::to_path(item.clpi_file_name).stem().string())));

    mxdebug_if(m_debug_mpls, fmt::format("add_external_files_from_mpls: M2TS for sub_path {0}: {1}\n", sub_path_idx - 1, !m2ts.empty() ? m2ts.string() : "not found"));

    if (m2ts.empty())
      continue;

    try {
      auto in                           = std::make_shared<mm_file_io_c>(m2ts.string(), libebml::MODE_READ);
      auto file                         = std::make_shared<file_t>(std::static_pointer_cast<mm_io_c>(in));

      file->m_timestamp_restriction_min = item.in_time;
      file->m_timestamp_restriction_max = item.out_time;
      file->m_timestamp_mpls_sync       = item.sync_start_pts_of_playitem;

      m_files.push_back(file);

      mxdebug_if(m_debug_mpls, fmt::format("add_external_files_from_mpls: sub_path file num {0} name {1} ts_rec_min {2} ts_rec_max {3} ts_mpls_sync {4}\n",
                                           m_files.size() - 1, m2ts.string(), file->m_timestamp_restriction_min, file->m_timestamp_restriction_max, file->m_timestamp_mpls_sync));

    } catch (mtx::mm_io::exception &ex) {
      mxdebug_if(m_debug_mpls, fmt::format("add_external_files_from_mpls: could not open {0}: {1}\n", m2ts.string(), ex.error()));
    }
  }
}

int64_t
reader_c::get_progress() {
  return m_bytes_processed;
}

int64_t
reader_c::get_maximum_progress() {
  return m_bytes_to_process;
}

}
