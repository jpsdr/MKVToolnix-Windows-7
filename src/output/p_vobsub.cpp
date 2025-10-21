/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   VobSub packetizer

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <matroska/KaxTracks.h>

#include "common/codec.h"
#include "common/compression.h"
#include "common/spu.h"
#include "common/strings/formatting.h"
#include "input/subtitles.h"
#include "output/p_vobsub.h"

vobsub_packetizer_c::vobsub_packetizer_c(generic_reader_c *reader,
                                         track_info_c &ti)
  : generic_packetizer_c{reader, ti, track_subtitle}
{
  set_default_compression_method(COMPRESSION_ZLIB);
}

vobsub_packetizer_c::~vobsub_packetizer_c() {
}

void
vobsub_packetizer_c::set_headers() {
  set_codec_id(MKV_S_VOBSUB);
  set_codec_private(m_ti.m_private_data);

  generic_packetizer_c::set_headers();

  m_track_entry->EnableLacing(false);
}

void
vobsub_packetizer_c::process_impl(packet_cptr const &packet) {
  add_packet(packet);
}

connection_result_e
vobsub_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                    std::string &) {
  vobsub_packetizer_c *vsrc;

  vsrc = dynamic_cast<vobsub_packetizer_c *>(src);
  if (!vsrc)
    return CAN_CONNECT_NO_FORMAT;
  return CAN_CONNECT_YES;
}

void
vobsub_packetizer_c::after_packet_timestamped(packet_t &packet) {
  static debugging_option_c debug{"spu|spu_duration"};

  if (!packet.has_duration() || (0 == packet.duration)) {
    auto new_duration = mtx::spu::get_duration(packet.data->get_buffer(), packet.data->get_size());
    packet.duration   = new_duration.to_ns(0);

    mxdebug_if(debug, fmt::format("vobsub: no duration at the container level; setting to {0} from SPU packets\n", mtx::string::format_timestamp(new_duration.to_ns(0))));

    return;
  }

  auto current_duration = mtx::spu::get_duration(packet.data->get_buffer(), packet.data->get_size());
  auto diff             = current_duration.valid() ? (current_duration - timestamp_c::ns(packet.duration)).abs() : timestamp_c::ns(0);

  if (diff >= timestamp_c::ms(1)) {
    mxdebug_if(debug,
               fmt::format("vobsub: setting SPU duration to {0} (existing duration: {1}, difference: {2})\n",
                           mtx::string::format_timestamp(packet.duration), mtx::string::format_timestamp(current_duration.to_ns(0)), mtx::string::format_timestamp(diff)));
    mtx::spu::set_duration(packet.data->get_buffer(), packet.data->get_size(), timestamp_c::ns(packet.duration));
  }
}
