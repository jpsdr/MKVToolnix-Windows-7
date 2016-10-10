/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   MPEG TS (transport stream) demultiplexer module

   Written by Massimo Callegari <massimocallegari@yahoo.it>
   and Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <iostream>

#include "common/at_scope_exit.h"
#include "common/bswap.h"
#include "common/checksums/crc.h"
#include "common/checksums/base_fwd.h"
#include "common/clpi.h"
#include "common/endian.h"
#include "common/math.h"
#include "common/mp3.h"
#include "common/mm_mpls_multi_file_io.h"
#include "common/ac3.h"
#include "common/id_info.h"
#include "common/iso639.h"
#include "common/list_utils.h"
#include "common/mpeg1_2.h"
#include "common/mpeg4_p2.h"
#include "common/strings/formatting.h"
#include "input/aac_framing_packet_converter.h"
#include "input/r_mpeg_ts.h"
#include "input/teletext_to_srt_packet_converter.h"
#include "input/truehd_ac3_splitting_packet_converter.h"
#include "output/p_aac.h"
#include "output/p_ac3.h"
#include "output/p_avc.h"
#include "output/p_dts.h"
#include "output/p_hdmv_pgs.h"
#include "output/p_hevc_es.h"
#include "output/p_mp3.h"
#include "output/p_mpeg1_2.h"
#include "output/p_pcm.h"
#include "output/p_textsubs.h"
#include "output/p_truehd.h"
#include "output/p_vc1.h"

// This is ISO/IEC 13818-1.

#define TS_CONSECUTIVE_PACKETS 16
#define TS_PROBE_SIZE          (2 * TS_CONSECUTIVE_PACKETS * 204)
#define TS_PACKET_SIZE         188
#define TS_MAX_PACKET_SIZE     204

int mpeg_ts_reader_c::potential_packet_sizes[] = { 188, 192, 204, 0 };

// ------------------------------------------------------------

void
mpeg_ts_track_c::process(packet_cptr const &packet) {
  if (!converter || !converter->convert(packet))
    reader.m_reader_packetizers[ptzr]->process(packet);
}

void
mpeg_ts_track_c::send_to_packetizer() {
  auto timestamp_to_use   = !m_timestamp.valid()                                               ? timestamp_c{}
                          : reader.m_dont_use_audio_pts && (ES_AUDIO_TYPE == type)             ? timestamp_c{}
                          : m_apply_dts_timestamp_fix && (m_previous_timestamp == m_timestamp) ? timestamp_c{}
                          :                                                                      std::max(m_timestamp, reader.m_global_timestamp_offset);

  auto timestamp_to_check = timestamp_to_use.valid() ? timestamp_to_use : reader.m_stream_timestamp;
  auto const &min         = reader.get_timecode_restriction_min();
  auto const &max         = reader.get_timecode_restriction_max();
  auto use_packet         = ptzr != -1;

  if (   (min.valid() && (timestamp_to_check < min))
      || (max.valid() && (timestamp_to_check > max)))
    use_packet = false;

  if (timestamp_to_use.valid()) {
    reader.m_stream_timestamp  = timestamp_to_use;
    timestamp_to_use          -= std::max(reader.m_global_timestamp_offset, min.valid() ? min : timestamp_c::ns(0));
  }

  if (timestamp_to_use.valid()) {
    if (mtx::included_in(type, ES_VIDEO_TYPE, ES_AUDIO_TYPE))
      reader.m_last_non_subtitle_timestamp = timestamp_to_use;

    else if (   (type == ES_SUBT_TYPE)
             && reader.m_last_non_subtitle_timestamp.valid()
             && ((timestamp_to_use - reader.m_last_non_subtitle_timestamp).abs() >= timestamp_c::s(5)))
      timestamp_to_use = reader.m_last_non_subtitle_timestamp;
  }

  mxdebug_if(m_debug_delivery, boost::format("send_to_packetizer: PID %1% expected %2% actual %3% timestamp_to_use %4% m_previous_timestamp %5%\n")
             % pid % pes_payload_size_to_read % pes_payload_read->get_size() % timestamp_to_use % m_previous_timestamp);

  if (use_packet) {
    auto bytes_to_skip = std::min<size_t>(pes_payload_read->get_size(), skip_packet_data_bytes);
    // if (pid == 4113)
      // mxdebug(boost::format("YADDA PID %1% PES payload read %2% skip_packet_data_bytes %3% TS to use %4% current TS %5% previous TS %6% global TS offset %7% checksum %|8$08x|\n")
      //         % pid % pes_payload_read->get_size() % skip_packet_data_bytes % timestamp_to_use % m_timestamp % m_previous_timestamp % reader.m_global_timestamp_offset
      //         % mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, pes_payload_read->get_buffer() + bytes_to_skip, pes_payload_read->get_size() - bytes_to_skip));
    process(std::make_shared<packet_t>(memory_c::clone(pes_payload_read->get_buffer() + bytes_to_skip, pes_payload_read->get_size() - bytes_to_skip), timestamp_to_use.to_ns(-1)));
  }

  clear_pes_payload();
  processed                          = false;
  reader.m_packet_sent_to_packetizer = true;
  m_previous_timestamp               = m_timestamp;
  m_timestamp.reset();
}

bool
mpeg_ts_track_c::is_pes_payload_complete()
  const {
  return !is_pes_payload_size_unbounded()
      && pes_payload_size_to_read
      && !remaining_payload_size_to_read();
}

bool
mpeg_ts_track_c::is_pes_payload_size_unbounded()
  const {
  return pes_payload_size_to_read == offsetof(mpeg_ts_pes_header_t, flags1);
}

void
mpeg_ts_track_c::add_pes_payload(unsigned char *ts_payload,
                                 size_t ts_payload_size) {
  auto to_add = is_pes_payload_size_unbounded() ? ts_payload_size : std::min(ts_payload_size, remaining_payload_size_to_read());

  if (to_add)
    pes_payload_read->add(ts_payload, to_add);
}

void
mpeg_ts_track_c::add_pes_payload_to_probe_data() {
  if (!m_probe_data)
    m_probe_data = byte_buffer_cptr(new byte_buffer_c);
  m_probe_data->add(pes_payload_read->get_buffer(), pes_payload_read->get_size());
}

void
mpeg_ts_track_c::clear_pes_payload() {
  pes_payload_read->clear();
  pes_payload_size_to_read = 0;
}

int
mpeg_ts_track_c::new_stream_v_mpeg_1_2() {
  if (!m_m2v_parser) {
    m_m2v_parser = std::shared_ptr<M2VParser>(new M2VParser);
    m_m2v_parser->SetProbeMode();
    m_m2v_parser->SetThrowOnError(true);
  }

  m_m2v_parser->WriteData(pes_payload_read->get_buffer(), pes_payload_read->get_size());

  int state = m_m2v_parser->GetState();
  if (state != MPV_PARSER_STATE_FRAME) {
    mxdebug_if(m_debug_headers, boost::format("new_stream_v_mpeg_1_2: no valid frame in %1% bytes\n") % pes_payload_read->get_size());
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
  v_frame_rate   = seq_hdr.frameOrFieldRate;
  v_aspect_ratio = seq_hdr.aspectRatio;

  if ((0 >= v_aspect_ratio) || (1 == v_aspect_ratio))
    v_dwidth = v_width;
  else
    v_dwidth = (int)(v_height * v_aspect_ratio);
  v_dheight  = v_height;

  MPEGChunk *raw_seq_hdr_chunk = m_m2v_parser->GetRealSequenceHeader();
  if (raw_seq_hdr_chunk) {
    mxdebug_if(m_debug_headers, boost::format("new_stream_v_mpeg_1_2: sequence header size: %1%\n") % raw_seq_hdr_chunk->GetSize());
    raw_seq_hdr = memory_c::clone(raw_seq_hdr_chunk->GetPointer(), raw_seq_hdr_chunk->GetSize());
  }

  mxdebug_if(m_debug_headers, boost::format("new_stream_v_mpeg_1_2: width: %1%, height: %2%\n") % v_width % v_height);
  if (v_width == 0 || v_height == 0)
    return FILE_STATUS_MOREDATA;

  return 0;
}

int
mpeg_ts_track_c::new_stream_v_avc() {
  if (!m_avc_parser) {
    m_avc_parser = mpeg4::p10::avc_es_parser_cptr(new mpeg4::p10::avc_es_parser_c);
    m_avc_parser->ignore_nalu_size_length_errors();

    if (mtx::includes(reader.m_ti.m_nalu_size_lengths, reader.tracks.size()))
      m_avc_parser->set_nalu_size_length(reader.m_ti.m_nalu_size_lengths[0]);
    else if (mtx::includes(reader.m_ti.m_nalu_size_lengths, -1))
      m_avc_parser->set_nalu_size_length(reader.m_ti.m_nalu_size_lengths[-1]);
  }

  mxdebug_if(m_debug_headers, boost::format("new_stream_v_avc: packet size: %1%\n") % pes_payload_read->get_size());

  m_avc_parser->add_bytes(pes_payload_read->get_buffer(), pes_payload_read->get_size());

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
mpeg_ts_track_c::new_stream_v_hevc() {
  if (!m_hevc_parser)
    m_hevc_parser = std::make_shared<mtx::hevc::es_parser_c>();

  m_hevc_parser->add_bytes(pes_payload_read->get_buffer(), pes_payload_read->get_size());

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

  return 0;
}

int
mpeg_ts_track_c::new_stream_v_vc1() {
  if (!m_vc1_parser)
    m_vc1_parser = std::make_shared<vc1::es_parser_c>();

  m_vc1_parser->add_bytes(pes_payload_read->get_buffer(), pes_payload_read->get_size());

  if (!m_vc1_parser->is_sequence_header_available())
    return FILE_STATUS_MOREDATA;

  auto seqhdr = vc1::sequence_header_t{};
  m_vc1_parser->get_sequence_header(seqhdr);

  v_width  = seqhdr.pixel_width;
  v_height = seqhdr.pixel_height;

  return 0;
}

int
mpeg_ts_track_c::new_stream_a_mpeg() {
  add_pes_payload_to_probe_data();

  mp3_header_t header;

  int offset = find_mp3_header(m_probe_data->get_buffer(), m_probe_data->get_size());
  if (-1 == offset)
    return FILE_STATUS_MOREDATA;

  decode_mp3_header(m_probe_data->get_buffer() + offset, &header);
  a_channels    = header.channels;
  a_sample_rate = header.sampling_frequency;
  codec         = header.get_codec();

  mxdebug_if(m_debug_headers, boost::format("new_stream_a_mpeg: Channels: %1%, sample rate: %2%\n") %a_channels % a_sample_rate);
  return 0;
}

int
mpeg_ts_track_c::new_stream_a_aac() {
  add_pes_payload_to_probe_data();

  auto parser = aac::parser_c{};
  parser.add_bytes(m_probe_data->get_buffer(), m_probe_data->get_size());
  if (!parser.frames_available() || !parser.headers_parsed())
    return FILE_STATUS_MOREDATA;

  m_aac_frame = parser.get_frame();

  mxdebug_if(reader.m_debug_aac, boost::format("new_stream_a_aac: first AAC header: %1%\n") % m_aac_frame.to_string());

  a_channels    = m_aac_frame.m_header.channels;
  a_sample_rate = m_aac_frame.m_header.sample_rate;

  return 0;
}

int
mpeg_ts_track_c::new_stream_a_ac3() {
  add_pes_payload_to_probe_data();

  ac3::parser_c parser;
  parser.add_bytes(m_probe_data->get_buffer(), m_probe_data->get_size());
  if (!parser.frame_available())
    return FILE_STATUS_MOREDATA;

  ac3::frame_c header = parser.get_frame();

  mxdebug_if(m_debug_headers,
             boost::format("new_stream_a_ac3: first ac3 header bsid %1% channels %2% sample_rate %3% bytes %4% samples %5%\n")
             % header.m_bs_id % header.m_channels % header.m_sample_rate % header.m_bytes % header.m_samples);

  a_channels    = header.m_channels;
  a_sample_rate = header.m_sample_rate;
  a_bsid        = header.m_bs_id;

  return 0;
}

int
mpeg_ts_track_c::new_stream_a_dts() {
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
mpeg_ts_track_c::new_stream_a_pcm() {
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
             boost::format("new_stream_a_pcm: header: 0x%|1$08x| channels: %2%, sample rate: %3%, bits per channel: %4%\n")
             % get_uint32_be(buffer) % a_channels % a_sample_rate % a_bits_per_sample);

  return a_sample_rate ? 0 : FILE_STATUS_DONE;
}

int
mpeg_ts_track_c::new_stream_a_truehd() {
  if (!m_truehd_parser)
    m_truehd_parser = truehd_parser_cptr(new truehd_parser_c);

  m_truehd_parser->add_data(pes_payload_read->get_buffer(), pes_payload_read->get_size());
  clear_pes_payload();

  while (m_truehd_parser->frame_available() && (!m_truehd_found_truehd || !m_truehd_found_ac3)) {
    auto frame = m_truehd_parser->get_next_frame();

    if (frame->is_ac3()) {
      if (!m_truehd_found_ac3) {
        m_truehd_found_ac3 = true;
        auto &header       = frame->m_ac3_header;

        mxdebug_if(m_debug_headers,
                   boost::format("new_stream_a_truehd: first AC-3 header bsid %1% channels %2% sample_rate %3% bytes %4% samples %5%\n")
                   % header.m_bs_id % header.m_channels % header.m_sample_rate % header.m_bytes % header.m_samples);

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
               boost::format("new_stream_a_truehd: first TrueHD header channels %1% sampling_rate %2% samples_per_frame %3%\n")
               % frame->m_channels % frame->m_sampling_rate % frame->m_samples_per_frame);

    codec                 = frame->codec();
    a_channels            = frame->m_channels;
    a_sample_rate         = frame->m_sampling_rate;
    m_truehd_found_truehd = true;
  }

  return m_truehd_found_truehd && m_truehd_found_ac3 ? 0 : FILE_STATUS_MOREDATA;
}

bool
mpeg_ts_track_c::has_packetizer()
  const {
  return -1 != ptzr;
}

void
mpeg_ts_track_c::set_pid(uint16_t new_pid) {
  pid = new_pid;

  std::string arg;
  m_debug_delivery = debugging_c::requested("mpeg_ts")
                   || (   debugging_c::requested("mpeg_ts_delivery", &arg)
                      && (arg.empty() || (arg == to_string(pid))));

  m_debug_timestamp_wrapping = debugging_c::requested("mpeg_ts")
                            || (   debugging_c::requested("mpeg_ts_timestamp_wrapping", &arg)
                               && (arg.empty() || (arg == to_string(pid))));
}

bool
mpeg_ts_track_c::detect_timestamp_wrap(timestamp_c &timestamp) {
  static auto const s_wrap_limit = timestamp_c::mpeg(1 << 30);

  if (   !timestamp.valid()
      || !m_previous_valid_timestamp.valid()
      || ((timestamp - m_previous_valid_timestamp).abs() < s_wrap_limit))
    return false;
  return true;
}

void
mpeg_ts_track_c::adjust_timestamp_for_wrap(timestamp_c &timestamp) {
  static auto const s_wrap_limit = timestamp_c::mpeg(1ll << 30);
  static auto const s_bad_limit  = timestamp_c::m(5);

  if (!timestamp.valid())
    return;

  if (timestamp < s_wrap_limit)
    timestamp += m_timestamp_wrap_add;

  // For subtitle tracks only detect jumps backwards in time, not
  // forward. Subtitles often have holes larger than five minutes
  // between the entries.
  if (   ((timestamp < m_previous_valid_timestamp) || (ES_SUBT_TYPE != type))
      && ((timestamp - m_previous_valid_timestamp).abs() >= s_bad_limit))
    timestamp = timestamp_c{};
}

void
mpeg_ts_track_c::handle_timestamp_wrap(timestamp_c &pts,
                                      timestamp_c &dts) {
  static auto const s_wrap_add    = timestamp_c::mpeg(1ll << 33);
  static auto const s_wrap_limit  = timestamp_c::mpeg(1ll << 30);
  static auto const s_reset_limit = timestamp_c::h(1);

  if (!m_timestamps_wrapped) {
    m_timestamps_wrapped = detect_timestamp_wrap(pts) || detect_timestamp_wrap(dts);
    if (m_timestamps_wrapped) {
      m_timestamp_wrap_add += s_wrap_add;
      mxdebug_if(m_debug_timestamp_wrapping,
                 boost::format("handle_timestamp_wrap: Timestamp wrapping detected for PID %1% pts %2% dts %3% previous_valid %4% global_offset %5% new wrap_add %6%\n")
                 % pid % pts % dts % m_previous_valid_timestamp % reader.m_global_timestamp_offset % m_timestamp_wrap_add);

    }

  } else if (pts.valid() && (pts < s_wrap_limit) && (pts > s_reset_limit)) {
    m_timestamps_wrapped = false;
    mxdebug_if(m_debug_timestamp_wrapping,
               boost::format("handle_timestamp_wrap: Timestamp wrapping reset for PID %1% pts %2% dts %3% previous_valid %4% global_offset %5% current wrap_add %6%\n")
               % pid % pts % dts % m_previous_valid_timestamp % reader.m_global_timestamp_offset % m_timestamp_wrap_add);
  }

  adjust_timestamp_for_wrap(pts);
  adjust_timestamp_for_wrap(dts);

  // mxinfo(boost::format("pid %5% PTS before %1% now %2% wrapped %3% reset prevvalid %4% diff %6%\n") % before % pts % m_timestamps_wrapped % m_previous_valid_timestamp % pid % (pts - m_previous_valid_timestamp));
}

bool
mpeg_ts_track_c::parse_ac3_pmt_descriptor(mpeg_ts_pmt_descriptor_t const &,
                                          mpeg_ts_pmt_pid_info_t const &pmt_pid_info) {
  if (pmt_pid_info.stream_type != ISO_13818_PES_PRIVATE)
    return false;

  type  = ES_AUDIO_TYPE;
  codec = codec_c::look_up(codec_c::type_e::A_AC3);

  return true;
}

bool
mpeg_ts_track_c::parse_dts_pmt_descriptor(mpeg_ts_pmt_descriptor_t const &,
                                          mpeg_ts_pmt_pid_info_t const &pmt_pid_info) {
  if (pmt_pid_info.stream_type != ISO_13818_PES_PRIVATE)
    return false;

  type  = ES_AUDIO_TYPE;
  codec = codec_c::look_up(codec_c::type_e::A_DTS);

  return true;
}

bool
mpeg_ts_track_c::parse_srt_pmt_descriptor(mpeg_ts_pmt_descriptor_t const &pmt_descriptor,
                                          mpeg_ts_pmt_pid_info_t const &pmt_pid_info) {
  if (pmt_pid_info.stream_type != ISO_13818_PES_PRIVATE)
    return false;

  auto buffer      = reinterpret_cast<unsigned char const *>(&pmt_descriptor + 1);
  auto num_entries = static_cast<unsigned int>(pmt_descriptor.length) / 5;
  mxdebug_if(reader.m_debug_pat_pmt, boost::format("parse_srt_pmt_descriptor: Teletext PMT descriptor, %1% entries\n") % num_entries);
  for (auto idx = 0u; idx < num_entries; ++idx) {
    // Bits:
    //  0–23: ISO 639 language code
    // 24–28: teletext type
    // 29-31: teletext magazine number
    // 32-35: teletext page number (units)
    // 36-39: teletext page number (tens)

    // Teletext type is:
    //   0: ?
    //   1: teletext
    //   2: teletext subtitles
    //   3: teletext additional information
    //   4: teletext program schedule
    //   5: teletext subtitles: hearing impaired

    bit_reader_c r{buffer, 5};

    r.skip_bits(24);
    auto ttx_type     = r.get_bits(5);
    auto ttx_magazine = r.get_bits(3);
    auto ttx_page     = r.get_bits(4) * 10 + r.get_bits(4);
    ttx_page          = (ttx_magazine ? ttx_magazine : 8) * 100 + ttx_page;

    if (reader.m_debug_pat_pmt) {
      mxdebug(boost::format(" %1%: language %2% type %3% magazine %4% page %5%\n")
              % idx % std::string(reinterpret_cast<char const *>(buffer), 3) % ttx_type % ttx_magazine % ttx_page);
    }

    if (!mtx::included_in(ttx_type, 2u, 5u)) {
      buffer += 5;
      continue;
    }

    mpeg_ts_track_c *to_set_up{};

    if (!m_ttx_wanted_page) {
      converter.reset(new teletext_to_srt_packet_converter_c{});
      to_set_up = this;

    } else {
      auto new_track = std::make_shared<mpeg_ts_track_c>(reader);
      m_coupled_tracks.emplace_back(new_track);

      to_set_up            = new_track.get();
      to_set_up->converter = converter;
      to_set_up->set_pid(pid);
    }

    to_set_up->parse_iso639_language_from(buffer);
    to_set_up->m_ttx_wanted_page.reset(ttx_page);

    to_set_up->type      = ES_SUBT_TYPE;
    to_set_up->codec     = codec_c::look_up(codec_c::type_e::S_SRT);
    to_set_up->probed_ok = true;

    buffer += 5;
  }

  return true;
}

bool
mpeg_ts_track_c::parse_registration_pmt_descriptor(mpeg_ts_pmt_descriptor_t const &pmt_descriptor,
                                                   mpeg_ts_pmt_pid_info_t const &pmt_pid_info) {
  if (pmt_pid_info.stream_type != ISO_13818_PES_PRIVATE)
    return false;

  if (pmt_descriptor.length < 4)
    return false;

  auto fourcc = fourcc_c{reinterpret_cast<unsigned char const *>(&pmt_descriptor + 1)};
  auto reg_codec  = codec_c::look_up(fourcc.str());

  mxdebug_if(reader.m_debug_pat_pmt, boost::format("parse_registration_pmt_descriptor: Registration descriptor with FourCC: %1% codec: %2%\n") % fourcc.description() % reg_codec);

  if (!reg_codec.valid())
    return false;

  switch (reg_codec.get_track_type()) {
    case track_audio:    type = ES_AUDIO_TYPE; break;
    case track_video:    type = ES_VIDEO_TYPE; break;
    case track_subtitle: type = ES_SUBT_TYPE;  break;
    default:
      return false;
  }

  codec = reg_codec;

  return true;
}

bool
mpeg_ts_track_c::parse_vobsub_pmt_descriptor(mpeg_ts_pmt_descriptor_t const &pmt_descriptor,
                                             mpeg_ts_pmt_pid_info_t const &pmt_pid_info) {
  if (pmt_pid_info.stream_type != ISO_13818_PES_PRIVATE)
    return false;

  type  = ES_SUBT_TYPE;
  codec = codec_c::look_up(codec_c::type_e::S_VOBSUB);

  if (pmt_descriptor.length >= 8)
    // Bits:
    //  0–23: ISO 639 language code
    // 24–31: subtitling type
    // 32–47: composition page ID
    // 48–63: ancillary page ID
    parse_iso639_language_from(&pmt_descriptor + 1);

  return true;
}

void
mpeg_ts_track_c::parse_iso639_language_from(void const *buffer) {
  auto value        = std::string{ reinterpret_cast<char const *>(buffer), 3 };
  auto language_idx = map_to_iso639_2_code(balg::to_lower_copy(value));
  if (-1 != language_idx)
    language = g_iso639_languages[language_idx].iso639_2_code;
}

std::size_t
mpeg_ts_track_c::remaining_payload_size_to_read()
  const {
  return pes_payload_size_to_read - std::min(pes_payload_size_to_read, pes_payload_read->get_size());
}

// ------------------------------------------------------------

bool
mpeg_ts_reader_c::probe_file(mm_io_c *in,
                             uint64_t) {
  auto mpls_in   = dynamic_cast<mm_mpls_multi_file_io_c *>(in);
  auto in_to_use = mpls_in ? static_cast<mm_io_c *>(mpls_in) : in;

  if (in_to_use->get_size() < 4)
    return -1;

  auto packet_size = detect_packet_size(in_to_use, in_to_use->get_size());

  if (packet_size == -1)
    return false;

  // Check for a h.264/h.265 start code right at the beginning in
  // order to avoid false positives.
  in_to_use->setFilePointer(0);
  auto marker = in_to_use->read_uint32_be();

  if ((marker == 0x00000001u) || ((marker >> 8) == 0x00000001u))
    return false;

  return true;
}

int
mpeg_ts_reader_c::detect_packet_size(mm_io_c *in,
                                     uint64_t size) {
  try {
    size        = std::min(static_cast<uint64_t>(TS_PROBE_SIZE), size);
    auto buffer = memory_c::alloc(size);
    auto mem    = buffer->get_buffer();

    in->setFilePointer(0, seek_beginning);
    size = in->read(mem, size);

    std::vector<int> positions;
    for (size_t i = 0; i < size; ++i)
      if (0x47 == mem[i])
        positions.push_back(i);

    for (size_t i = 0; positions.size() > i; ++i) {
      for (size_t k = 0; 0 != potential_packet_sizes[k]; ++k) {
        unsigned int pos            = positions[i];
        unsigned int packet_size    = potential_packet_sizes[k];
        unsigned int num_startcodes = 1;

        while ((TS_CONSECUTIVE_PACKETS > num_startcodes) && (pos < size) && (0x47 == mem[pos])) {
          pos += packet_size;
          ++num_startcodes;
        }

        if (TS_CONSECUTIVE_PACKETS <= num_startcodes)
          return packet_size;
      }
    }
  } catch (...) {
  }

  return -1;
}

mpeg_ts_reader_c::mpeg_ts_reader_c(const track_info_c &ti,
                                   const mm_io_cptr &in)
  : generic_reader_c(ti, in)
  , PAT_found{}
  , PMT_found{}
  , PMT_pid(-1)
  , es_to_process{}
  , m_global_timestamp_offset{}
  , m_stream_timestamp{timestamp_c::ns(0)}
  , m_state{ps_probing}
  , m_probe_range{}
  , file_done{}
  , m_packet_sent_to_packetizer{}
  , m_dont_use_audio_pts{      "mpeg_ts|mpeg_ts_dont_use_audio_pts"}
  , m_debug_resync{            "mpeg_ts|mpeg_ts_resync"}
  , m_debug_pat_pmt{           "mpeg_ts|mpeg_ts_pat|mpeg_ts_pmt|mpeg_ts_headers"}
  , m_debug_headers{           "mpeg_ts|mpeg_ts_headers"}
  , m_debug_packet{            "mpeg_ts|mpeg_ts_packet"}
  , m_debug_aac{               "mpeg_ts|mpeg_aac"}
  , m_debug_timestamp_wrapping{"mpeg_ts|mpeg_ts_timestamp_wrapping"}
  , m_debug_clpi{              "clpi"}
  , m_detected_packet_size{}
  , m_num_pat_crc_errors{}
  , m_num_pmt_crc_errors{}
  , m_validate_pat_crc{true}
  , m_validate_pmt_crc{true}
{
  auto mpls_in = dynamic_cast<mm_mpls_multi_file_io_c *>(get_underlying_input());
  if (mpls_in)
    m_chapter_timestamps = mpls_in->get_chapters();
}

void
mpeg_ts_reader_c::read_headers() {
  try {
    m_probe_range          = calculate_probe_range(m_in->get_size(), 10 * 1024 * 1024);
    size_t size_to_probe   = std::min(m_size, m_probe_range);
    m_detected_packet_size = detect_packet_size(m_in.get(), size_to_probe);

    m_in->setFilePointer(0);

    mxdebug_if(m_debug_headers, boost::format("read_headers: Starting to build PID list. (packet size: %1%)\n") % m_detected_packet_size);

    auto PAT = std::make_shared<mpeg_ts_track_c>(*this);
    PAT->type = PAT_TYPE;
    tracks.push_back(PAT);

    unsigned char buf[TS_MAX_PACKET_SIZE]; // maximum TS packet size + 1

    while (true) {
      if (m_in->read(buf, m_detected_packet_size) != static_cast<unsigned int>(m_detected_packet_size))
        break;

      if (buf[0] != 0x47) {
        if (resync(m_in->getFilePointer() - m_detected_packet_size))
          continue;
        break;
      }

      parse_packet(buf);

      if (PAT_found && PMT_found && (0 == es_to_process))
        break;

      auto eof = m_in->eof() || (m_in->getFilePointer() >= size_to_probe);
      if (!eof)
        continue;

      // Determine if we haven't found a PAT or a PMT but have plenty
      // of CRC errors (e.g. for badly mastered discs). In such a case
      // we should read from the start again, this time ignoring the
      // errors for the specific type.
      mxdebug_if(m_debug_headers,
                 boost::format("read_headers: EOF during detection. #tracks %1% #PAT CRC errors %2% #PMT CRC errors %3% PAT found %4% PMT found %5%\n")
                 % tracks.size() % m_num_pat_crc_errors % m_num_pmt_crc_errors % PAT_found % PMT_found);

      if (!PAT_found && m_validate_pat_crc)
        m_validate_pat_crc = false;

      else if (PAT_found && !PMT_found && m_validate_pmt_crc)
        m_validate_pmt_crc = false;

      else
        break;

      m_in->setFilePointer(0);
      m_in->clear_eof();

      tracks.clear();
      auto PAT = std::make_shared<mpeg_ts_track_c>(*this);
      PAT->type = PAT_TYPE;
      tracks.push_back(PAT);
    }
  } catch (...) {
    mxdebug_if(m_debug_headers, boost::format("read_headers: caught exception\n"));
  }

  mxdebug_if(m_debug_headers, boost::format("read_headers: Detection done on %1% bytes\n") % m_in->getFilePointer());

  m_in->setFilePointer(0, seek_beginning); // rewind file for later remux

  parse_clip_info_file();
  process_chapter_entries();

  for (auto &track : tracks) {
    track->clear_pes_payload();
    track->processed        = false;
    // track->timestamp_offset = -1;

    for (auto const &coupled_track : track->m_coupled_tracks)
      if (coupled_track->language.empty())
        coupled_track->language = track->language;

    // For TrueHD tracks detection for embedded AC-3 frames is
    // done. However, "probed_ok" is only set on the TrueHD track if
    // both types have been found. If only TrueHD is found then
    // "probed_ok" must be set to true after detection has exhausted
    // the search space; otherwise a TrueHD-only track would never be
    // considered OK.
    if (track->codec.is(codec_c::type_e::A_TRUEHD) && track->m_truehd_found_truehd)
      track->probed_ok = true;
  }

  show_demuxer_info();
}

void
mpeg_ts_reader_c::determine_global_timestamp_offset() {
  m_state = ps_determining_timestamp_offset;

  m_in->setFilePointer(0);
  m_in->clear_eof();

  mxdebug_if(m_debug_headers, boost::format("determine_global_timestamp_offset: determining global timestamp offset from the first %1% bytes\n") % m_probe_range);

  try {
    unsigned char buf[TS_MAX_PACKET_SIZE]; // maximum TS packet size + 1

    while (m_in->getFilePointer() < m_probe_range) {
      if (m_in->read(buf, m_detected_packet_size) != static_cast<unsigned int>(m_detected_packet_size))
        break;

      if (buf[0] != 0x47) {
        if (resync(m_in->getFilePointer() - m_detected_packet_size))
          continue;
        break;
      }

      parse_packet(buf);
    }
  } catch (...) {
    mxdebug_if(m_debug_headers, boost::format("determine_global_timestamp_offset: caught exception\n"));
  }

  mxdebug_if(m_debug_headers, boost::format("determine_global_timestamp_offset: detection done; global timestamp offset is %1%\n") % m_global_timestamp_offset);

  m_in->setFilePointer(0);
  m_in->clear_eof();

  for (auto const &track : tracks) {
    track->clear_pes_payload();
    track->processed            = false;
    track->m_timestamps_wrapped = false;
    track->m_timestamp_wrap_add = timestamp_c::ns(0);
  }
}

void
mpeg_ts_reader_c::process_chapter_entries() {
  if (m_chapter_timestamps.empty() || m_ti.m_no_chapters)
    return;

  std::stable_sort(m_chapter_timestamps.begin(), m_chapter_timestamps.end());

  mm_mem_io_c out{nullptr, 0, 1000};
  out.set_file_name(m_ti.m_fname);
  out.write_bom("UTF-8");

  size_t idx = 0;
  for (auto &timestamp : m_chapter_timestamps) {
    ++idx;
    auto ms = timestamp.to_ms();
    out.puts(boost::format("CHAPTER%|1$02d|=%|2$02d|:%|3$02d|:%|4$02d|.%|5$03d|\n"
                           "CHAPTER%|1$02d|NAME=Chapter %1%\n")
             % idx
             % ( ms / 60 / 60 / 1000)
             % ((ms      / 60 / 1000) %   60)
             % ((ms           / 1000) %   60)
             % ( ms                   % 1000));
  }

  mm_text_io_c text_out(&out, false);
  try {
    m_chapters = parse_chapters(&text_out, 0, -1, 0, m_ti.m_chapter_language, "", true);
    align_chapter_edition_uids(m_chapters.get());

  } catch (mtx::chapter_parser_x &ex) {
  }
}

mpeg_ts_reader_c::~mpeg_ts_reader_c() {
}

uint32_t
mpeg_ts_reader_c::calculate_crc(void const *buffer,
                                size_t size)
  const {
  return mtx::bswap_32(mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::crc32_ieee, buffer, size, 0xffffffff));
}

void
mpeg_ts_reader_c::identify() {
  auto info    = mtx::id::info_c{};
  auto mpls_in = dynamic_cast<mm_mpls_multi_file_io_c *>(get_underlying_input());
  if (mpls_in)
    mpls_in->create_verbose_identification_info(info);

  id_result_container(info.get());

  size_t i;
  for (i = 0; i < tracks.size(); i++) {
    mpeg_ts_track_ptr &track = tracks[i];

    if (!track->probed_ok || !track->codec)
      continue;

    info = mtx::id::info_c{};
    info.add(mtx::id::language, track->language);
    info.set(mtx::id::ts_pid,   track->pid);
    info.set(mtx::id::number,   track->pid);

    if (ES_AUDIO_TYPE == track->type) {
      info.add(mtx::id::audio_channels,           track->a_channels);
      info.add(mtx::id::audio_sampling_frequency, track->a_sample_rate);
      info.add(mtx::id::audio_bits_per_sample,    track->a_bits_per_sample);

    } else if (ES_VIDEO_TYPE == track->type)
      info.add(mtx::id::pixel_dimensions, boost::format("%1%x%2%") % track->v_width % track->v_height);

    else if (ES_SUBT_TYPE == track->type) {
      info.set(mtx::id::text_subtitles, track->codec.is(codec_c::type_e::S_SRT));
      if (track->m_ttx_wanted_page)
        info.set(mtx::id::teletext_page, *track->m_ttx_wanted_page);
    }

    std::string type = ES_AUDIO_TYPE == track->type ? ID_RESULT_TRACK_AUDIO
                     : ES_VIDEO_TYPE == track->type ? ID_RESULT_TRACK_VIDEO
                     :                                ID_RESULT_TRACK_SUBTITLES;

    id_result_track(i, type, track->codec.get_name(), info.get());
  }

  if (!m_chapter_timestamps.empty())
    id_result_chapters(m_chapter_timestamps.size());
}

bool
mpeg_ts_reader_c::parse_pat(mpeg_ts_track_c &track) {
  if (track.pes_payload_read->get_size() < sizeof(mpeg_ts_pat_t)) {
    mxdebug_if(m_debug_pat_pmt, "Invalid parameters!\n");
    return false;
  }

  auto pat        = track.pes_payload_read->get_buffer();
  auto pat_header = reinterpret_cast<mpeg_ts_pat_t *>(pat);

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

  unsigned short pat_section_length = pat_header->get_section_length();
  uint32_t elapsed_CRC              = calculate_crc(pat, 3 + pat_section_length - 4/*CRC32*/);
  uint32_t read_CRC                 = get_uint32_be(pat + 3 + pat_section_length - 4);

  if (elapsed_CRC != read_CRC) {
    mxdebug_if(m_debug_pat_pmt, boost::format("parse_pat: Wrong PAT CRC !!! Elapsed = 0x%|1$08x|, read 0x%|2$08x|, validate PAT CRC? %3%\n") % elapsed_CRC % read_CRC % m_validate_pat_crc);
    ++m_num_pat_crc_errors;
    if (m_validate_pat_crc)
      return false;
  }

  if (pat_section_length < 13 || pat_section_length > 1021) {
    mxdebug_if(m_debug_pat_pmt, boost::format("parse_pat: Wrong PAT section_length (= %1%)\n") % pat_section_length);
    return false;
  }

  auto prog_count  = static_cast<unsigned int>(pat_section_length - 5 - 4/*CRC32*/) / 4;
  auto pat_section = reinterpret_cast<mpeg_ts_pat_section_t *>(pat + sizeof(mpeg_ts_pat_t));

  for (auto i = 0u; i < prog_count; i++, pat_section++) {
    unsigned short local_program_number = pat_section->get_program_number();
    uint16_t tmp_pid                    = pat_section->get_pid();

    mxdebug_if(m_debug_pat_pmt, boost::format("parse_pat: program_number: %1%; %2%_pid: %3%\n")
               % local_program_number
               % (0 == local_program_number ? "nit" : "pmt")
               % tmp_pid);

    if (0 != local_program_number) {
      PAT_found = true;

      bool skip = false;
      for (uint16_t i = 0; i < tracks.size(); i++) {
        if (tracks[i]->pid == tmp_pid) {
          skip = true;
          break;
        }
      }

      if (skip == true)
        continue;

      auto PMT       = std::make_shared<mpeg_ts_track_c>(*this);
      PMT->type      = PMT_TYPE;
      PMT->processed = false;
      es_to_process  = 0;

      PMT->set_pid(tmp_pid);

      tracks.push_back(PMT);
    }
  }

  return true;
}

bool
mpeg_ts_reader_c::parse_pmt(mpeg_ts_track_c &track) {
  if (track.pes_payload_read->get_size() < sizeof(mpeg_ts_pmt_t)) {
    mxdebug_if(m_debug_pat_pmt, "Invalid parameters!\n");
    return false;
  }

  auto pmt        = track.pes_payload_read->get_buffer();
  auto pmt_header = reinterpret_cast<mpeg_ts_pmt_t *>(pmt);

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

  unsigned short pmt_section_length = pmt_header->get_section_length();
  uint32_t elapsed_CRC              = calculate_crc(pmt, 3 + pmt_section_length - 4/*CRC32*/);
  uint32_t read_CRC                 = get_uint32_be(pmt + 3 + pmt_section_length - 4);

  if (elapsed_CRC != read_CRC) {
    mxdebug_if(m_debug_pat_pmt, boost::format("parse_pmt: Wrong PMT CRC !!! Elapsed = 0x%|1$08x|, read 0x%|2$08x|, validate PMT CRC? %3%\n") % elapsed_CRC % read_CRC % m_validate_pmt_crc);
    ++m_num_pmt_crc_errors;
    if (m_validate_pmt_crc)
      return false;
  }

  if (pmt_section_length < 13 || pmt_section_length > 1021) {
    mxdebug_if(m_debug_pat_pmt, boost::format("parse_pmt: Wrong PMT section_length (=%1%)\n") % pmt_section_length);
    return false;
  }

  auto pmt_descriptor      = reinterpret_cast<mpeg_ts_pmt_descriptor_t *>(pmt + sizeof(mpeg_ts_pmt_t));
  auto program_info_length = pmt_header->get_program_info_length();

  while (pmt_descriptor < (mpeg_ts_pmt_descriptor_t *)(pmt + sizeof(mpeg_ts_pmt_t) + program_info_length))
    pmt_descriptor = (mpeg_ts_pmt_descriptor_t *)((unsigned char *)pmt_descriptor + sizeof(mpeg_ts_pmt_descriptor_t) + pmt_descriptor->length);

  mpeg_ts_pmt_pid_info_t *pmt_pid_info = (mpeg_ts_pmt_pid_info_t *)pmt_descriptor;

  // Calculate pids_count
  size_t pids_found = 0;
  while (pmt_pid_info < (mpeg_ts_pmt_pid_info_t *)(pmt + 3 + pmt_section_length - 4/*CRC32*/)) {
    pids_found++;
    pmt_pid_info = (mpeg_ts_pmt_pid_info_t *)((unsigned char *)pmt_pid_info + sizeof(mpeg_ts_pmt_pid_info_t) + pmt_pid_info->get_es_info_length());
  }

  mxdebug_if(m_debug_pat_pmt, boost::format("parse_pmt: program number     (%1%)\n") % pmt_header->get_program_number());
  mxdebug_if(m_debug_pat_pmt, boost::format("parse_pmt: pcr pid            (%1%)\n") % pmt_header->get_pcr_pid());

  if (pids_found == 0) {
    mxdebug_if(m_debug_pat_pmt, "There's no information about elementary PIDs\n");
    return 0;
  }

  pmt_pid_info = (mpeg_ts_pmt_pid_info_t *)pmt_descriptor;

  // Extract pid_info
  while (pmt_pid_info < (mpeg_ts_pmt_pid_info_t *)(pmt + 3 + pmt_section_length - 4/*CRC32*/)) {
    mpeg_ts_track_ptr track(new mpeg_ts_track_c(*this));
    unsigned short es_info_length = pmt_pid_info->get_es_info_length();
    track->type                   = ES_UNKNOWN;

    track->set_pid(pmt_pid_info->get_pid());

    switch(pmt_pid_info->stream_type) {
      case ISO_11172_VIDEO:
      case ISO_13818_VIDEO:
        track->type      = ES_VIDEO_TYPE;
        track->codec     = codec_c::look_up(codec_c::type_e::V_MPEG12);
        break;
      case ISO_14496_PART2_VIDEO:
        track->type      = ES_VIDEO_TYPE;
        track->codec     = codec_c::look_up(codec_c::type_e::V_MPEG4_P2);
        break;
      case ISO_14496_PART10_VIDEO:
        track->type      = ES_VIDEO_TYPE;
        track->codec     = codec_c::look_up(codec_c::type_e::V_MPEG4_P10);
        break;
      case ISO_23008_PART2_VIDEO:
        track->type      = ES_VIDEO_TYPE;
        track->codec     = codec_c::look_up(codec_c::type_e::V_MPEGH_P2);
        break;
      case STREAM_VIDEO_VC1:
        track->type      = ES_VIDEO_TYPE;
        track->codec     = codec_c::look_up(codec_c::type_e::V_VC1);
        break;
      case ISO_11172_AUDIO:
      case ISO_13818_AUDIO:
        track->type      = ES_AUDIO_TYPE;
        track->codec     = codec_c::look_up(codec_c::type_e::A_MP3);
        break;
      case ISO_13818_PART7_AUDIO:
      case ISO_14496_PART3_AUDIO:
        track->type      = ES_AUDIO_TYPE;
        track->codec     = codec_c::look_up(codec_c::type_e::A_AAC);
        break;
      case STREAM_AUDIO_PCM:
        track->type      = ES_AUDIO_TYPE;
        track->codec     = codec_c::look_up(codec_c::type_e::A_PCM);
        break;

      case STREAM_AUDIO_AC3_LOSSLESS: {
        auto ac3_track         = std::make_shared<mpeg_ts_track_c>(*this);
        ac3_track->type        = ES_AUDIO_TYPE;
        ac3_track->codec       = codec_c::look_up(codec_c::type_e::A_AC3);
        ac3_track->converter   = std::make_shared<truehd_ac3_splitting_packet_converter_c>();
        ac3_track->set_pid(pmt_pid_info->get_pid());

        track->type            = ES_AUDIO_TYPE;
        track->codec           = codec_c::look_up(codec_c::type_e::A_TRUEHD);
        track->converter       = ac3_track->converter;
        track->m_coupled_tracks.push_back(ac3_track);

        break;
      }

      case STREAM_AUDIO_AC3:
      case STREAM_AUDIO_EAC3:      // E-AC-3
      case STREAM_AUDIO_EAC3_2:    // E-AC-3 secondary stream
      case STREAM_AUDIO_EAC3_ATSC: // E-AC-3 as defined in ATSC A/52:2012 Annex G
        track->type      = ES_AUDIO_TYPE;
        track->codec     = codec_c::look_up(codec_c::type_e::A_AC3);
        break;
      case STREAM_AUDIO_DTS:
      case STREAM_AUDIO_DTS_HD:
      case STREAM_AUDIO_DTS_HD_MA:
      case STREAM_AUDIO_DTS_HD2:
        track->type      = ES_AUDIO_TYPE;
        track->codec     = codec_c::look_up(codec_c::type_e::A_DTS);
        break;
      case STREAM_SUBTITLES_HDMV_PGS:
        track->type      = ES_SUBT_TYPE;
        track->codec     = codec_c::look_up(codec_c::type_e::S_HDMV_PGS);
        track->probed_ok = true;
        break;
      case ISO_13818_PES_PRIVATE:
        break;
      default:
        mxdebug_if(m_debug_pat_pmt, boost::format("parse_pmt: Unknown stream type: %1%\n") % (int)pmt_pid_info->stream_type);
        track->type      = ES_UNKNOWN;
        break;
    }

    pmt_descriptor   = (mpeg_ts_pmt_descriptor_t *)((unsigned char *)pmt_pid_info + sizeof(mpeg_ts_pmt_pid_info_t));
    bool missing_tag = true;

    while (pmt_descriptor < (mpeg_ts_pmt_descriptor_t *)((unsigned char *)pmt_pid_info + sizeof(mpeg_ts_pmt_pid_info_t) + es_info_length)) {
      mxdebug_if(m_debug_pat_pmt, boost::format("parse_pmt: PMT descriptor tag 0x%|1$02x| length %2%\n") % static_cast<unsigned int>(pmt_descriptor->tag) % static_cast<unsigned int>(pmt_descriptor->length));

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
          track->parse_srt_pmt_descriptor(*pmt_descriptor, *pmt_pid_info);
          break;
        case 0x59: // Subtitles descriptor
          track->parse_vobsub_pmt_descriptor(*pmt_descriptor, *pmt_pid_info);
          break;
        case 0x6A: // AC-3 descriptor
        case 0x7A: // E-AC-3 descriptor
          track->parse_ac3_pmt_descriptor(*pmt_descriptor, *pmt_pid_info);
          break;
        case 0x7b: // DTS descriptor
          track->parse_dts_pmt_descriptor(*pmt_descriptor, *pmt_pid_info);
          break;
      }

      pmt_descriptor = (mpeg_ts_pmt_descriptor_t *)((unsigned char *)pmt_descriptor + sizeof(mpeg_ts_pmt_descriptor_t) + pmt_descriptor->length);
    }

    // Default to AC-3 if it's a PES private stream type that's missing
    // a known/more concrete descriptor tag.
    if ((pmt_pid_info->stream_type == ISO_13818_PES_PRIVATE) && missing_tag) {
      track->type  = ES_AUDIO_TYPE;
      track->codec = codec_c::look_up(codec_c::type_e::A_AC3);
    }

    if (track->type != ES_UNKNOWN) {
      PMT_found        = true;
      track->processed = false;
      tracks.push_back(track);
      ++es_to_process;

      brng::copy(track->m_coupled_tracks, std::back_inserter(tracks));
      es_to_process += track->m_coupled_tracks.size();
    }

    mxdebug_if(m_debug_pat_pmt,
               boost::format("parse_pmt: PID %1% stream type %|2$02x| has type: %3%\n")
               % track->pid
               % static_cast<unsigned int>(pmt_pid_info->stream_type)
               % (track->type != ES_UNKNOWN ? track->codec.get_name() : "<unknown>"));

    pmt_pid_info = (mpeg_ts_pmt_pid_info_t *)((unsigned char *)pmt_pid_info + sizeof(mpeg_ts_pmt_pid_info_t) + es_info_length);
  }

  return true;
}

void
mpeg_ts_reader_c::parse_pes(mpeg_ts_track_c &track) {
  at_scope_exit_c finally{[&track]() { track.clear_pes_payload(); }};

  auto pes            = track.pes_payload_read->get_buffer();
  auto const pes_size = track.pes_payload_read->get_size();
  auto pes_header     = reinterpret_cast<mpeg_ts_pes_header_t *>(pes);

  if (pes_size < sizeof(mpeg_ts_pes_header_t)) {
    mxdebug_if(m_debug_packet, boost::format("parse_pes: error: PES payload (%1%) too small for PES header structure itself (%2%)\n") % pes_size % sizeof(mpeg_ts_pes_header_t));
    return;
  }

  auto has_pts = (pes_header->get_pts_dts_flags() & 0x02) == 0x02; // 10 and 11 mean PTS is present
  auto has_dts = (pes_header->get_pts_dts_flags() & 0x01) == 0x01; // 01 and 11 mean DTS is present
  auto to_skip = offsetof(mpeg_ts_pes_header_t, pes_header_data_length) + pes_header->pes_header_data_length + 1;

  if (pes_size < to_skip) {
    mxdebug_if(m_debug_packet, boost::format("parse_pes: error: PES payload (%1%) too small for PES header + header data including PTS/DTS (%2%)\n") % pes_size % to_skip);
    return;
  }

  timestamp_c pts, dts, orig_pts, orig_dts;
  if (has_pts) {
    pts = read_timestamp(&pes_header->pts_dts);
    dts = pts;
  }

  if (has_dts)
    dts = read_timestamp(&pes_header->pts_dts + (has_pts ? 5 : 0));

  if (!track.m_use_dts)
    dts = pts;

  orig_pts = pts;
  orig_dts = dts;

  track.handle_timestamp_wrap(pts, dts);

  if (ps_determining_timestamp_offset == m_state) {
    if (   dts.valid()
        && (   !m_global_timestamp_offset.valid()
            || (dts < m_global_timestamp_offset)))
      m_global_timestamp_offset = dts;
    return;
  }

  if (m_debug_packet) {
    mxdebug(boost::format("parse_pes: PES info at file position %1%:\n") % (m_in->getFilePointer() - m_detected_packet_size));
    mxdebug(boost::format("parse_pes:    stream_id = %1% PID = %2%\n") % static_cast<unsigned int>(pes_header->stream_id) % track.pid);
    mxdebug(boost::format("parse_pes:    PES_packet_length = %1%, PES_header_data_length = %2%, data starts at %3%\n") % pes_size % static_cast<unsigned int>(pes_header->pes_header_data_length) % to_skip);
    mxdebug(boost::format("parse_pes:    PTS? %1% (%5% processed %6%) DTS? (%7% processed %8%) %2% ESCR = %3% ES_rate = %4%\n")
            % has_pts % has_dts % static_cast<unsigned int>(pes_header->get_escr()) % static_cast<unsigned int>(pes_header->get_es_rate()) % orig_pts % pts % orig_dts % dts);
    mxdebug(boost::format("parse_pes:    DSM_trick_mode = %1%, add_copy = %2%, CRC = %3%, ext = %4%\n")
            % static_cast<unsigned int>(pes_header->get_dsm_trick_mode()) % static_cast<unsigned int>(pes_header->get_additional_copy_info()) % static_cast<unsigned int>(pes_header->get_pes_crc())
            % static_cast<unsigned int>(pes_header->get_pes_extension()));
  }

  if (pts.valid()) {
    track.m_previous_valid_timestamp = pts;

    if (!m_global_timestamp_offset.valid() || (dts < m_global_timestamp_offset)) {
      mxdebug_if(m_debug_headers, boost::format("new global timestamp offset %1% prior %2% file position afterwards %3%\n") % dts % m_global_timestamp_offset % m_in->getFilePointer());
      m_global_timestamp_offset = dts;
    }

    track.m_timestamp = dts;

    mxdebug_if(m_debug_packet, boost::format("parse_pes: PID %1% PTS found: %1% DTS: %3%\n") % track.pid % pts % dts);
  }

  track.pes_payload_read->remove(to_skip);

  if (ps_probing == m_state)
    probe_packet_complete(track);

  else
    track.send_to_packetizer();
}

timestamp_c
mpeg_ts_reader_c::read_timestamp(unsigned char *p) {
  int64_t mpeg_timestamp  =  static_cast<int64_t>(             ( p[0]   >> 1) & 0x07) << 30;
  mpeg_timestamp         |= (static_cast<int64_t>(get_uint16_be(&p[1])) >> 1)         << 15;
  mpeg_timestamp         |=  static_cast<int64_t>(get_uint16_be(&p[3]))               >>  1;

  return timestamp_c::mpeg(mpeg_timestamp);
}

void
mpeg_ts_reader_c::parse_packet(unsigned char *buf) {
  auto hdr   = reinterpret_cast<mpeg_ts_packet_header_t *>(buf);
  auto track = find_track_for_pid(hdr->get_pid());

  if (   hdr->has_transport_error() // corrupted packet
      || !hdr->has_payload()        // no ts_payload
      || !track
      || track->processed)
    return;

  auto payload = determine_ts_payload_start(hdr);

  if (payload.second)
    handle_ts_payload(*track, *hdr, payload.first, payload.second);
}

void
mpeg_ts_reader_c::handle_ts_payload(mpeg_ts_track_c &track,
                                    mpeg_ts_packet_header_t &ts_header,
                                    unsigned char *ts_payload,
                                    std::size_t ts_payload_size) {
  if (mtx::included_in(track.type, PAT_TYPE, PMT_TYPE))
    handle_pat_pmt_payload(track, ts_header, ts_payload, ts_payload_size);

  else if (mtx::included_in(track.type, ES_VIDEO_TYPE, ES_AUDIO_TYPE, ES_SUBT_TYPE))
    handle_pes_payload(track, ts_header, ts_payload, ts_payload_size);

  else {
    mxdebug_if(m_debug_headers, boost::format("handle_ts_payload: error: unknown track.type %1%\n") % static_cast<unsigned int>(track.type));
  }
}

void
mpeg_ts_reader_c::handle_pat_pmt_payload(mpeg_ts_track_c &track,
                                         mpeg_ts_packet_header_t &ts_header,
                                         unsigned char *ts_payload,
                                         std::size_t ts_payload_size) {
  if (ps_probing != m_state)
    return;

  if (ts_header.is_payload_unit_start()) {
    auto to_skip = static_cast<std::size_t>(ts_payload[0]) + 1;

    if (to_skip >= ts_payload_size) {
      mxdebug_if(m_debug_headers, boost::format("handle_pat_pmt_payload: error: TS payload size %1% too small; needed to skip %2%\n") % ts_payload_size % to_skip);
      track.pes_payload_size_to_read = 0;
      return;
    }

    track.clear_pes_payload();
    auto pat_table                   = reinterpret_cast<mpeg_ts_pat_t *>(ts_payload + to_skip);
    ts_payload_size                 -= to_skip;
    ts_payload                      += to_skip;
    track.pes_payload_size_to_read  = pat_table->get_section_length() + 3;
  }

  track.add_pes_payload(ts_payload, ts_payload_size);

  if (track.is_pes_payload_complete())
    probe_packet_complete(track);
}

void
mpeg_ts_reader_c::handle_pes_payload(mpeg_ts_track_c &track,
                                     mpeg_ts_packet_header_t &ts_header,
                                     unsigned char *ts_payload,
                                     std::size_t ts_payload_size) {
  if (ts_header.is_payload_unit_start()) {
    if (track.is_pes_payload_size_unbounded() && track.pes_payload_read->get_size())
      parse_pes(track);

    if (track.pes_payload_size_to_read && track.pes_payload_read->get_size() && m_debug_headers)
      mxdebug(boost::format("handle_pes_payload: error: dropping incomplete PES payload; target size %1% already read %2%\n") % track.pes_payload_size_to_read % track.pes_payload_read->get_size());

    track.clear_pes_payload();

    if (ts_payload_size < sizeof(mpeg_ts_pes_header_t)) {
      mxdebug_if(m_debug_headers, boost::format("handle_pes_payload: error: TS payload size %1% too small for PES header %2%\n") % ts_payload_size % sizeof(mpeg_ts_pes_header_t));
      return;
    }

    auto pes_header                = reinterpret_cast<mpeg_ts_pes_header_t *>(ts_payload);
    track.pes_payload_size_to_read = pes_header->get_pes_packet_length() + offsetof(mpeg_ts_pes_header_t, flags1);
  }

  track.add_pes_payload(ts_payload, ts_payload_size);

  if (track.is_pes_payload_complete())
    parse_pes(track);
}

int
mpeg_ts_reader_c::determine_track_parameters(mpeg_ts_track_c &track) {
  if (track.type == PAT_TYPE)
    return parse_pat(track) ? 0 : -1;

  else if (track.type == PMT_TYPE)
    return parse_pmt(track) ? 0 : -1;

  else if (track.type == ES_VIDEO_TYPE) {
    if (track.codec.is(codec_c::type_e::V_MPEG12))
      return track.new_stream_v_mpeg_1_2();
    else if (track.codec.is(codec_c::type_e::V_MPEG4_P10))
      return track.new_stream_v_avc();
    else if (track.codec.is(codec_c::type_e::V_MPEGH_P2))
      return track.new_stream_v_hevc();
    else if (track.codec.is(codec_c::type_e::V_VC1))
      return track.new_stream_v_vc1();

    track.pes_payload_read->set_chunk_size(512 * 1024);

  } else if (track.type != ES_AUDIO_TYPE)
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
mpeg_ts_reader_c::probe_packet_complete(mpeg_ts_track_c &track) {
  int result = -1;

  try {
    result = determine_track_parameters(track);
  } catch (...) {
  }

  track.clear_pes_payload();

  if (result != 0) {
    mxdebug_if(m_debug_headers, (result == FILE_STATUS_MOREDATA) ? "probe_packet_complete: Need more data to probe ES\n" : "probe_packet_complete: Failed to parse packet. Reset and retry\n");
    track.processed = false;

    return;
  }

  if (track.type == PAT_TYPE || track.type == PMT_TYPE) {
    auto it = brng::find_if(tracks, [&track](mpeg_ts_track_ptr const &candidate) { return candidate.get() == &track; });
    if (tracks.end() != it)
      tracks.erase(it);

  } else {
    track.processed = true;
    track.probed_ok = true;
    es_to_process--;
    mxdebug_if(m_debug_headers, boost::format("probe_packet_complete: ES to process: %1%\n") % es_to_process);
  }
}

void
mpeg_ts_reader_c::create_packetizer(int64_t id) {
  if ((0 > id) || (tracks.size() <= static_cast<size_t>(id)))
    return;

  mpeg_ts_track_ptr &track = tracks[id];
  char type                = ES_AUDIO_TYPE == track->type ? 'a'
                           : ES_SUBT_TYPE  == track->type ? 's'
                           :                                'v';

  if (!track->probed_ok || (0 == track->ptzr) || !demuxing_requested(type, id, track->language))
    return;

  m_ti.m_id       = id;
  m_ti.m_language = track->language;

  if (ES_AUDIO_TYPE == track->type) {
    if (track->codec.is(codec_c::type_e::A_MP3)) {
      track->ptzr = add_packetizer(new mp3_packetizer_c(this, m_ti, track->a_sample_rate, track->a_channels, (0 != track->a_sample_rate) && (0 != track->a_channels)));
      show_packetizer_info(id, PTZR(track->ptzr));

    } else if (track->codec.is(codec_c::type_e::A_AAC))
      create_aac_audio_packetizer(track);

    else if (track->codec.is(codec_c::type_e::A_AC3))
      create_ac3_audio_packetizer(track);

    else if (track->codec.is(codec_c::type_e::A_DTS)) {
      track->ptzr = add_packetizer(new dts_packetizer_c(this, m_ti, track->a_dts_header));
      show_packetizer_info(id, PTZR(track->ptzr));

    } else if (track->codec.is(codec_c::type_e::A_PCM)) {
      track->ptzr = add_packetizer(new pcm_packetizer_c(this, m_ti, track->a_sample_rate, track->a_channels, track->a_bits_per_sample, pcm_packetizer_c::big_endian_integer));
      show_packetizer_info(id, PTZR(track->ptzr));

    } else if (track->codec.is(codec_c::type_e::A_TRUEHD))
      create_truehd_audio_packetizer(track);

  } else if (ES_VIDEO_TYPE == track->type) {
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

  else if (track->codec.is(codec_c::type_e::S_SRT))
    create_srt_subtitles_packetizer(track);

  if (-1 != track->ptzr)
    m_ptzr_to_track_map[PTZR(track->ptzr)] = track;
}

void
mpeg_ts_reader_c::create_aac_audio_packetizer(mpeg_ts_track_ptr const &track) {
  auto aac_packetizer = new aac_packetizer_c(this, m_ti, track->m_aac_frame.m_header.profile, track->m_aac_frame.m_header.sample_rate, track->m_aac_frame.m_header.channels, true);
  track->ptzr         = add_packetizer(aac_packetizer);
  track->converter.reset(new aac_framing_packet_converter_c{PTZR(track->ptzr)});

  if (AAC_PROFILE_SBR == track->m_aac_frame.m_header.profile)
    aac_packetizer->set_audio_output_sampling_freq(track->m_aac_frame.m_header.sample_rate * 2);
  show_packetizer_info(m_ti.m_id, aac_packetizer);
}

void
mpeg_ts_reader_c::create_ac3_audio_packetizer(mpeg_ts_track_ptr const &track) {
  track->ptzr = add_packetizer(new ac3_packetizer_c(this, m_ti, track->a_sample_rate, track->a_channels, track->a_bsid));
  show_packetizer_info(m_ti.m_id, PTZR(track->ptzr));

  if (track->converter)
    dynamic_cast<truehd_ac3_splitting_packet_converter_c &>(*track->converter).set_ac3_packetizer(PTZR(track->ptzr));
}

void
mpeg_ts_reader_c::create_truehd_audio_packetizer(mpeg_ts_track_ptr const &track) {
  track->ptzr = add_packetizer(new truehd_packetizer_c(this, m_ti, truehd_frame_t::truehd, track->a_sample_rate, track->a_channels));
  show_packetizer_info(m_ti.m_id, PTZR(track->ptzr));

  if (track->converter)
    dynamic_cast<truehd_ac3_splitting_packet_converter_c &>(*track->converter).set_packetizer(PTZR(track->ptzr));
}

void
mpeg_ts_reader_c::create_mpeg1_2_video_packetizer(mpeg_ts_track_ptr &track) {
  m_ti.m_private_data = track->raw_seq_hdr;
  auto m2vpacketizer  = new mpeg1_2_video_packetizer_c(this, m_ti, track->v_version, track->v_frame_rate, track->v_width, track->v_height, track->v_dwidth, track->v_dheight, false);
  track->ptzr         = add_packetizer(m2vpacketizer);

  show_packetizer_info(m_ti.m_id, PTZR(track->ptzr));
  m2vpacketizer->set_video_interlaced_flag(track->v_interlaced);
}

void
mpeg_ts_reader_c::create_mpeg4_p10_es_video_packetizer(mpeg_ts_track_ptr &track) {
  generic_packetizer_c *ptzr = new mpeg4_p10_es_video_packetizer_c(this, m_ti);
  track->ptzr                = add_packetizer(ptzr);
  ptzr->set_video_pixel_dimensions(track->v_width, track->v_height);

  show_packetizer_info(m_ti.m_id, PTZR(track->ptzr));
}

void
mpeg_ts_reader_c::create_mpegh_p2_es_video_packetizer(mpeg_ts_track_ptr &track) {
  auto ptzr   = new hevc_es_video_packetizer_c(this, m_ti);
  track->ptzr = add_packetizer(ptzr);
  ptzr->set_video_pixel_dimensions(track->v_width, track->v_height);

  show_packetizer_info(m_ti.m_id, PTZR(track->ptzr));
}

void
mpeg_ts_reader_c::create_vc1_video_packetizer(mpeg_ts_track_ptr &track) {
  track->m_use_dts = true;
  track->ptzr      = add_packetizer(new vc1_video_packetizer_c(this, m_ti));
  show_packetizer_info(m_ti.m_id, PTZR(track->ptzr));
}

void
mpeg_ts_reader_c::create_hdmv_pgs_subtitles_packetizer(mpeg_ts_track_ptr &track) {
  auto ptzr = new hdmv_pgs_packetizer_c(this, m_ti);
  ptzr->set_aggregate_packets(true);
  track->ptzr = add_packetizer(ptzr);

  show_packetizer_info(m_ti.m_id, PTZR(track->ptzr));
}

void
mpeg_ts_reader_c::create_srt_subtitles_packetizer(mpeg_ts_track_ptr const &track) {
  track->ptzr = add_packetizer(new textsubs_packetizer_c(this, m_ti, MKV_S_TEXTUTF8, false, true));

  auto &converter = dynamic_cast<teletext_to_srt_packet_converter_c &>(*track->converter);

  converter.demux_page(*track->m_ttx_wanted_page, PTZR(track->ptzr));
  converter.override_encoding(*track->m_ttx_wanted_page, track->language);

  show_packetizer_info(m_ti.m_id, PTZR(track->ptzr));
}

void
mpeg_ts_reader_c::create_packetizers() {
  determine_global_timestamp_offset();

  m_state = ps_muxing;

  mxdebug_if(m_debug_headers, boost::format("create_packetizers: create packetizers...\n"));
  for (std::size_t i = 0u, end = tracks.size(); i < end; ++i)
    create_packetizer(i);
}

void
mpeg_ts_reader_c::add_available_track_ids() {
  size_t i;

  for (i = 0; i < tracks.size(); i++)
    if (tracks[i]->probed_ok)
      add_available_track_id(i);
}

file_status_e
mpeg_ts_reader_c::finish() {
  if (file_done)
    return flush_packetizers();

  for (auto &track : tracks) {
    if ((-1 != track->ptzr) && (0 < track->pes_payload_read->get_size()))
      parse_pes(*track);

    if (track->converter)
      track->converter->flush();
  }

  file_done = true;

  return flush_packetizers();
}

file_status_e
mpeg_ts_reader_c::read(generic_packetizer_c *requested_ptzr,
                       bool force) {
  int64_t num_queued_bytes = get_queued_bytes();
  if (!force && (20 * 1024 * 1024 < num_queued_bytes)) {
    mpeg_ts_track_ptr requested_ptzr_track = m_ptzr_to_track_map[requested_ptzr];
    if (!requested_ptzr_track || ((ES_AUDIO_TYPE != requested_ptzr_track->type) && (ES_VIDEO_TYPE != requested_ptzr_track->type)) || (512 * 1024 * 1024 < num_queued_bytes))
      return FILE_STATUS_HOLDING;
  }

  unsigned char buf[TS_MAX_PACKET_SIZE + 1];

  if (file_done)
    return flush_packetizers();

  while (true) {
    if (m_in->read(buf, m_detected_packet_size) != static_cast<unsigned int>(m_detected_packet_size))
      return finish();

    if (buf[0] != 0x47) {
      if (resync(m_in->getFilePointer() - m_detected_packet_size))
        continue;
      return finish();
    }

    parse_packet(buf);

    if (m_packet_sent_to_packetizer)
      return FILE_STATUS_MOREDATA;
  }
}

bfs::path
mpeg_ts_reader_c::find_clip_info_file() {
  auto mpls_multi_in = dynamic_cast<mm_mpls_multi_file_io_c *>(get_underlying_input());
  auto clpi_file     = mpls_multi_in ? mpls_multi_in->get_file_names()[0] : bfs::path{m_ti.m_fname};

  clpi_file.replace_extension(".clpi");

  mxdebug_if(m_debug_clpi, boost::format("find_clip_info_file: Checking %1%\n") % clpi_file.string());

  if (bfs::exists(clpi_file))
    return clpi_file;

  bfs::path file_name(clpi_file.filename());
  bfs::path path(clpi_file.remove_filename());

  // clpi_file = path / ".." / file_name;
  // if (bfs::exists(clpi_file))
  //   return clpi_file;

  clpi_file = path / ".." / "clipinf" / file_name;
  mxdebug_if(m_debug_clpi, boost::format("find_clip_info_file: Checking %1%\n") % clpi_file.string());
  if (bfs::exists(clpi_file))
    return clpi_file;

  clpi_file = path / ".." / "CLIPINF" / file_name;
  mxdebug_if(m_debug_clpi, boost::format("find_clip_info_file: Checking %1%\n") % clpi_file.string());
  if (bfs::exists(clpi_file))
    return clpi_file;

  mxdebug_if(m_debug_clpi, "mpeg_ts_reader_c::find_clip_info_file: CLPI not found\n");

  return bfs::path();
}

void
mpeg_ts_reader_c::parse_clip_info_file() {
  bfs::path clpi_file(find_clip_info_file());
  if (clpi_file.empty())
    return;

  clpi::parser_c parser(clpi_file.string());
  if (!parser.parse())
    return;

  for (auto &track : tracks) {
    bool found = false;

    for (auto &program : parser.m_programs) {
      for (auto &stream : program->program_streams) {
        if ((stream->pid != track->pid) || stream->language.empty())
          continue;

        int language_idx = map_to_iso639_2_code(stream->language.c_str());
        if (-1 == language_idx)
          continue;

        track->language = g_iso639_languages[language_idx].iso639_2_code;
        found = true;
        break;
      }

      if (found)
        break;
    }
  }
}

bool
mpeg_ts_reader_c::resync(int64_t start_at) {
  try {
    mxdebug_if(m_debug_resync, boost::format("resync: Start resync for data from %1%\n") % start_at);
    m_in->setFilePointer(start_at);

    unsigned char buf[TS_MAX_PACKET_SIZE + 1];

    while (!m_in->eof()) {
      int64_t curr_pos = m_in->getFilePointer();
      buf[0] = m_in->read_uint8();

      if (0x47 != buf[0])
        continue;

      if (m_in->read(&buf[1], m_detected_packet_size) != static_cast<unsigned int>(m_detected_packet_size))
        return false;

      if (0x47 != buf[m_detected_packet_size]) {
        m_in->setFilePointer(curr_pos + 1);
        continue;
      }

      mxdebug_if(m_debug_resync, boost::format("resync: Re-established at %1%\n") % curr_pos);

      m_in->setFilePointer(curr_pos);
      return true;
    }

  } catch (...) {
    return false;
  }

  return false;
}

mpeg_ts_track_ptr
mpeg_ts_reader_c::find_track_for_pid(uint16_t pid)
  const {
  for (auto const &track : tracks)
    if (   (track->pid == pid)
        && (   mtx::included_in(m_state, ps_probing, ps_determining_timestamp_offset)
            || track->has_packetizer()))
      return track;

  return {};
}

std::pair<unsigned char *, std::size_t>
mpeg_ts_reader_c::determine_ts_payload_start(mpeg_ts_packet_header_t *hdr)
  const {
  auto ts_header_start  = reinterpret_cast<unsigned char *>(hdr);
  auto ts_payload_start = ts_header_start + sizeof(mpeg_ts_packet_header_t);

  if (hdr->has_adaptation_field())
    // Adaptation field's first byte is its length - 1.
    ts_payload_start += ts_payload_start[0] + 1;

  // Make sure not to overflow the TS packet buffer.
  ts_payload_start = std::min(ts_payload_start, ts_header_start + TS_PACKET_SIZE);

  return { ts_payload_start, static_cast<std::size_t>(ts_header_start + TS_PACKET_SIZE - ts_payload_start) };
}
