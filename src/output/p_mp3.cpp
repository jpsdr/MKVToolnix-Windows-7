/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   MP3 output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/codec.h"
#include "common/mp3.h"
#include "common/strings/formatting.h"
#include "merge/connection_checks.h"
#include "merge/output_control.h"
#include "merge/packet_extensions.h"
#include "output/p_mp3.h"

mp3_packetizer_c::mp3_packetizer_c(generic_reader_c *p_reader,
                                   track_info_c &p_ti,
                                   int samples_per_sec,
                                   int channels,
                                   bool source_is_good)
  : generic_packetizer_c{p_reader, p_ti, track_audio}
  , m_first_packet{true}
  , m_bytes_skipped(0)
  , m_samples_per_sec(samples_per_sec)
  , m_channels(channels)
  , m_samples_per_frame(1152)
  , m_byte_buffer(128 * 1024)
  , m_codec_id_set(false)
  , m_valid_headers_found(source_is_good)
  , m_timestamp_calculator{static_cast<int64_t>(samples_per_sec)}
  , m_packet_duration{m_timestamp_calculator.get_duration(m_samples_per_frame).to_ns()}
{
  set_track_default_duration(m_packet_duration);
  m_timestamp_calculator.set_allow_smaller_timestamps(true);
}

mp3_packetizer_c::~mp3_packetizer_c() {
}

void
mp3_packetizer_c::handle_garbage(int64_t bytes) {
  bool warning_printed = false;

  if (m_first_packet) {
    auto offset = calculate_avi_audio_sync(bytes, m_samples_per_frame, m_packet_duration);

    if (0 < offset) {
      mxinfo_tid(m_ti.m_fname, m_ti.m_id,
                 fmt::format(FY("This MPEG audio track contains {0} bytes of non-MP3 data at the beginning. "
                                "This corresponds to a delay of {1}ms. This delay will be used instead of the garbage data.\n"), bytes, offset / 1000000));
      warning_printed             = true;
      m_ti.m_tcsync.displacement += offset;
    }
  }

  if (!warning_printed)
    m_packet_extensions.push_back(std::make_shared<before_adding_to_cluster_cb_packet_extension_c>([this, bytes](packet_cptr const &packet, int64_t timestamp_offset) {
      mxwarn_tid(m_ti.m_fname, m_ti.m_id,
                 fmt::format("{0} {1}\n",
                             fmt::format(FNY("This audio track contains {0} byte of invalid data which was skipped before timestamp {1}.",
                                             "This audio track contains {0} bytes of invalid data which were skipped before timestamp {1}.", bytes),
                                         bytes, mtx::string::format_timestamp(packet->assigned_timestamp - timestamp_offset)),
                             Y("The audio/video synchronization may have been lost.")));
    }));
}

memory_cptr
mp3_packetizer_c::get_mp3_packet(mp3_header_t *mp3header) {
  if (m_byte_buffer.get_size() == 0)
    return {};

  int pos;
  while (1) {
    auto buf  = m_byte_buffer.get_buffer();
    auto size = m_byte_buffer.get_size();
    pos       = find_mp3_header(buf, size);

    if (0 > pos)
      return nullptr;

    decode_mp3_header(&buf[pos], mp3header);

    if ((pos + mp3header->framesize) > size)
      return nullptr;

    if (!mp3header->is_tag)
      break;

    m_byte_buffer.remove(mp3header->framesize + pos);
  }

  // Try to be smart. We might get fed trash before the very first MP3
  // header. And now a user has presented streams in which the trash
  // contains valid MP3 headers before the 'real' ones...
  // Screw the guys who program apps that use _random_ _trash_ for filling
  // gaps. Screw those who try to use AVI no matter the 'cost'!
  bool track_headers_changed = false;
  if (!m_valid_headers_found) {
    pos = find_consecutive_mp3_headers(m_byte_buffer.get_buffer(), m_byte_buffer.get_size(), 5);
    if (0 > pos)
      return nullptr;

    // Great, we have found five consecutive identical headers. Be happy
    // with those!
    decode_mp3_header(m_byte_buffer.get_buffer() + pos, mp3header);

    set_audio_channels(mp3header->channels);
    set_audio_sampling_freq(mp3header->sampling_frequency);

    m_samples_per_sec      = mp3header->sampling_frequency;
    track_headers_changed  = true;
    m_valid_headers_found  = true;
    m_bytes_skipped       += pos;
    if (0 < m_bytes_skipped)
      handle_garbage(m_bytes_skipped);
    m_byte_buffer.remove(pos);

    pos             = 0;
    m_bytes_skipped = 0;
  }

  m_bytes_skipped += pos;
  if (0 < m_bytes_skipped)
    handle_garbage(m_bytes_skipped);
  m_byte_buffer.remove(pos);
  pos             = 0;
  m_bytes_skipped = 0;

  if (m_first_packet) {
    m_samples_per_frame  = mp3header->samples_per_channel;
    m_packet_duration    = m_timestamp_calculator.get_duration(m_samples_per_frame).to_ns();
    std::string codec_id = MKV_A_MP3;
    codec_id[codec_id.length() - 1] = (char)(mp3header->layer + '0');
    set_codec_id(codec_id.c_str());

    m_timestamp_calculator.set_samples_per_second(m_samples_per_sec);
    set_track_default_duration(m_packet_duration);

    track_headers_changed = true;
  }

  if (track_headers_changed)
    rerender_track_headers();

  if (mp3header->framesize > m_byte_buffer.get_size())
    return nullptr;

  auto buf = memory_c::clone(m_byte_buffer.get_buffer(), mp3header->framesize);

  m_byte_buffer.remove(mp3header->framesize);

  return buf;
}

void
mp3_packetizer_c::set_headers() {
  if (!m_codec_id_set) {
    set_codec_id(MKV_A_MP3);
    m_codec_id_set = true;
  }
  set_audio_sampling_freq(m_samples_per_sec);
  set_audio_channels(m_channels);

  generic_packetizer_c::set_headers();
}

void
mp3_packetizer_c::process_impl(packet_cptr const &packet) {
  m_timestamp_calculator.add_timestamp(packet);
  m_discard_padding.add_maybe(packet->discard_padding);
  m_byte_buffer.add(packet->data->get_buffer(), packet->data->get_size());

  flush_packets();
}

void
mp3_packetizer_c::flush_packets() {
  mp3_header_t mp3header;
  memory_cptr mp3_packet;

  while ((mp3_packet = get_mp3_packet(&mp3header))) {
    auto new_timestamp = m_timestamp_calculator.get_next_timestamp(m_samples_per_frame);
    auto packet        = std::make_shared<packet_t>(mp3_packet, new_timestamp.to_ns(), m_packet_duration);

    packet->add_extensions(m_packet_extensions);
    packet->discard_padding = m_discard_padding.get_next().value_or(timestamp_c{});

    add_packet(packet);

    m_first_packet = false;
    m_packet_extensions.clear();
  }
}

connection_result_e
mp3_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                 std::string &error_message) {
  mp3_packetizer_c *msrc = dynamic_cast<mp3_packetizer_c *>(src);
  if (!msrc)
    return CAN_CONNECT_NO_FORMAT;

  connect_check_a_samplerate(m_samples_per_sec, msrc->m_samples_per_sec);
  connect_check_a_channels(m_channels, msrc->m_channels);

  return CAN_CONNECT_YES;
}
