/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   AC-3 output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/common_urls.h"
#include "common/ac3.h"
#include "common/codec.h"
#include "common/hacks.h"
#include "common/strings/formatting.h"
#include "merge/connection_checks.h"
#include "merge/output_control.h"
#include "merge/packet_extensions.h"
#include "output/p_ac3.h"

ac3_packetizer_c::ac3_packetizer_c(generic_reader_c *p_reader,
                                   track_info_c &p_ti,
                                   int samples_per_sec,
                                   int channels,
                                   int bsid)
  : generic_packetizer_c{p_reader, p_ti, track_audio}
  , m_timestamp_calculator{samples_per_sec}
  , m_samples_per_packet{1536}
  , m_packet_duration{m_timestamp_calculator.get_duration(m_samples_per_packet).to_ns()}
  , m_stream_position{}
  , m_first_packet{true}
  , m_remove_dialog_normalization_gain{get_option_for_track(m_ti.m_remove_dialog_normalization_gain, m_ti.m_id)}
  , m_verify_checksums{"ac3_verify_checksums"}
{
  m_first_ac3_header.m_sample_rate = samples_per_sec;
  m_first_ac3_header.m_bs_id       = bsid;
  m_first_ac3_header.m_channels    = channels;

  set_track_default_duration(m_packet_duration);
}

ac3_packetizer_c::~ac3_packetizer_c() {
}

mtx::ac3::frame_c
ac3_packetizer_c::get_frame() {
  mtx::ac3::frame_c frame = m_parser.get_frame();

  if (0 == frame.m_garbage_size)
    return frame;

  m_timestamp_calculator.drop_timestamps_before_position(frame.m_stream_position);

  bool warning_printed = false;
  if (m_first_packet) {
    auto offset = calculate_avi_audio_sync(frame.m_garbage_size, m_samples_per_packet, m_packet_duration);

    if (0 < offset) {
      mxinfo_tid(m_ti.m_fname, m_ti.m_id,
                 fmt::format(FY("This AC-3 track contains {0} bytes of non-AC-3 data at the beginning. "
                                "This corresponds to a delay of {1}ms. "
                                "This delay will be used instead of the non-AC-3 data.\n"),
                             frame.m_garbage_size, offset / 1000000));

      warning_printed             = true;
      m_ti.m_tcsync.displacement += offset;
    }
  }

  if (!warning_printed) {
    auto bytes = frame.m_garbage_size;
    m_packet_extensions.push_back(std::make_shared<before_adding_to_cluster_cb_packet_extension_c>([this, bytes](packet_cptr const &packet, int64_t timestamp_offset) {
      mxwarn_tid(m_ti.m_fname, m_ti.m_id,
                 fmt::format("{0} {1}\n",
                             fmt::format(FNY("This audio track contains {0} byte of invalid data which was skipped before timestamp {1}.",
                                             "This audio track contains {0} bytes of invalid data which were skipped before timestamp {1}.", bytes),
                                         bytes, mtx::string::format_timestamp(packet->assigned_timestamp - timestamp_offset)),
                             Y("The audio/video synchronization may have been lost.")));
    }));
  }

  return frame;
}

void
ac3_packetizer_c::set_headers() {
  std::string id = MKV_A_AC3;
  auto use_bsid  = mtx::hacks::is_engaged(mtx::hacks::KEEP_BSID910_IN_AC3_CODECID);

  if (m_first_ac3_header.is_eac3())
    id = MKV_A_EAC3;

  else if (use_bsid) {
    if (9 == m_first_ac3_header.m_bs_id)
      id += "/BSID9";
    else if (10 == m_first_ac3_header.m_bs_id)
      id += "/BSID10";
  }

  set_codec_id(id.c_str());
  set_audio_sampling_freq(m_first_ac3_header.m_sample_rate);
  set_audio_channels(m_first_ac3_header.m_channels);

  generic_packetizer_c::set_headers();
}

void
ac3_packetizer_c::process_impl(packet_cptr const &packet) {
  // mxinfo(fmt::format("tc {0} size {1}\n", mtx::string::format_timestamp(packet->timestamp), packet->data->get_size()));

  m_timestamp_calculator.add_timestamp(packet, m_stream_position);
  m_discard_padding.add_maybe(packet->discard_padding, m_stream_position);
  m_stream_position += packet->data->get_size();

  add_to_buffer(packet->data->get_buffer(), packet->data->get_size());

  flush_packets();
}

void
ac3_packetizer_c::set_timestamp_and_add_packet(packet_cptr const &packet,
                                              uint64_t packet_stream_position) {
  packet->timestamp = m_timestamp_calculator.get_next_timestamp(m_samples_per_packet, packet_stream_position).to_ns();
  packet->duration  = m_packet_duration;

  // if (packet_stream_position)
  //   mxinfo(fmt::format("  ts {0} position in {1} out {2}\n", mtx::string::format_timestamp(packet->timestamp), mtx::string::format_number(m_stream_position), mtx::string::format_number(*packet_stream_position)));

  auto ok_before = m_verify_checksums ? mtx::ac3::verify_checksums(packet->data->get_buffer(), packet->data->get_size(), true) : true;
  auto ok_after  = -1;

  if (m_remove_dialog_normalization_gain) {
    mtx::ac3::remove_dialog_normalization_gain(packet->data->get_buffer(), packet->data->get_size());
    if (m_verify_checksums)
      ok_after = mtx::ac3::verify_checksums(packet->data->get_buffer(), packet->data->get_size(), true) ? 1 : 0;
  }

  mxdebug_if(m_verify_checksums,
             fmt::format("AC-3 packetizer checksum verification at {0} / {1}: before/after removal: {2}/{3}\n",
                         mtx::string::format_timestamp(packet->timestamp), mtx::string::format_number(m_stream_position), ok_before, ok_after == -1 ? "n/a" : ok_after ? "1" : "0"));

  add_packet(packet);

  m_first_packet = false;
}

void
ac3_packetizer_c::add_to_buffer(uint8_t *const buf,
                                int size) {
  m_parser.add_bytes(buf, size);
}

void
ac3_packetizer_c::flush_impl() {
  m_parser.flush();
  flush_packets();
}

void
ac3_packetizer_c::flush_packets() {
  while (m_parser.frame_available()) {
    auto frame = get_frame();
    adjust_header_values(frame);

    auto packet = std::make_shared<packet_t>(frame.m_data);
    packet->add_extensions(m_packet_extensions);
    packet->discard_padding = m_discard_padding.get_next(frame.m_stream_position).value_or(timestamp_c{});

    set_timestamp_and_add_packet(packet, frame.m_stream_position);

    m_packet_extensions.clear();
  }
}

void
ac3_packetizer_c::adjust_header_values(mtx::ac3::frame_c const &ac3_header) {
  if (!m_first_packet)
    return;

  if (m_first_ac3_header.m_sample_rate != ac3_header.m_sample_rate)
    set_audio_sampling_freq(ac3_header.m_sample_rate);

  if (m_first_ac3_header.m_channels != ac3_header.m_channels)
    set_audio_channels(ac3_header.m_channels);

  if (ac3_header.is_eac3())
    set_codec_id(MKV_A_EAC3);

  if ((m_samples_per_packet != ac3_header.m_samples) || (m_first_ac3_header.m_sample_rate != ac3_header.m_sample_rate)) {
    if (ac3_header.m_sample_rate)
      m_timestamp_calculator.set_samples_per_second(ac3_header.m_sample_rate);

    if (ac3_header.m_samples)
      m_samples_per_packet = ac3_header.m_samples;

    m_packet_duration = m_timestamp_calculator.get_duration(m_samples_per_packet).to_ns();
    set_track_default_duration(m_packet_duration);
  }

  m_first_ac3_header = ac3_header;

  rerender_track_headers();
}

connection_result_e
ac3_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                 std::string &error_message) {
  ac3_packetizer_c *asrc = dynamic_cast<ac3_packetizer_c *>(src);
  if (!asrc)
    return CAN_CONNECT_NO_FORMAT;

  connect_check_a_samplerate(m_first_ac3_header.m_sample_rate, asrc->m_first_ac3_header.m_sample_rate);
  connect_check_a_channels(  m_first_ac3_header.m_channels,    asrc->m_first_ac3_header.m_channels);

  return CAN_CONNECT_YES;
}

ac3_bs_packetizer_c::ac3_bs_packetizer_c(generic_reader_c *p_reader,
                                         track_info_c &p_ti,
                                         unsigned long samples_per_sec,
                                         int channels,
                                         int bsid)
  : ac3_packetizer_c(p_reader, p_ti, samples_per_sec, channels, bsid)
  , m_bsb(0)
  , m_bsb_present(false)
{
}

static bool s_warning_printed = false;

void
ac3_bs_packetizer_c::add_to_buffer(uint8_t *const buf,
                                   int size) {
  if (((size % 2) == 1) && !s_warning_printed) {
    mxwarn(fmt::format(FY("ac3_bs_packetizer::add_to_buffer(): Untested code ('size' is odd). "
                          "If mkvmerge crashes or if the resulting file does not contain the complete and correct audio track, "
                          "then please open an issue over on {0}.\n"), MTX_URL_ISSUES));
    s_warning_printed = true;
  }

  uint8_t *sendptr;
  int size_add;
  bool new_bsb_present = false;

  if (m_bsb_present) {
    size_add = 1;
    sendptr  = buf + size + 1;
  } else {
    size_add = 0;
    sendptr  = buf + size;
  }

  size_add += size;
  if ((size_add % 2) == 1) {
    size_add--;
    sendptr--;
    new_bsb_present = true;
  }

  auto af_new_buffer = memory_c::alloc(size_add);
  auto new_buffer    = af_new_buffer->get_buffer();
  auto dptr          = new_buffer;
  auto sptr          = buf;

  if (m_bsb_present) {
    dptr[1]  = m_bsb;
    dptr[0]  = sptr[0];
    dptr    += 2;
    sptr++;
  }

  while (sptr < sendptr) {
    dptr[0]  = sptr[1];
    dptr[1]  = sptr[0];
    dptr    += 2;
    sptr    += 2;
  }

  if (new_bsb_present)
    m_bsb = *sptr;

  m_bsb_present = new_bsb_present;

  m_parser.add_bytes(new_buffer, size_add);
}
