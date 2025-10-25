/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/debugging.h"
#include "common/strings/parsing.h"
#include "merge/connection_checks.h"
#include "merge/packet_extensions.h"
#include "output/p_ssa.h"

ssa_packetizer_c::ssa_packetizer_c(generic_reader_c *p_reader,
                                   track_info_c &p_ti,
                                   const char *codec_id,
                                   bool recode)
  : textsubs_packetizer_c{p_reader, p_ti, codec_id, recode}
{
}

ssa_packetizer_c::~ssa_packetizer_c() {
}

void
ssa_packetizer_c::process_impl(packet_cptr const &packet) {
  packet->extensions.emplace_back(std::make_shared<before_adding_to_cluster_cb_packet_extension_c>([this](auto const &packet, auto) {
    this->fix_frame_number(packet);
  }));

  textsubs_packetizer_c::process_impl(packet);
}

void
ssa_packetizer_c::fix_frame_number(packet_cptr const &packet) {
  static debugging_option_c s_debug{"ssa_fix_frame_number"};

  auto entry     = packet->data->to_string();
  auto comma_pos = entry.find(',');

  if (!comma_pos || (std::string::npos == comma_pos))
    return;

  int frame_number{};
  if (!mtx::string::parse_number(entry.substr(0, comma_pos), frame_number))
    return;

  if (m_preceding_packetizer)
    frame_number += m_preceding_packetizer->m_highest_frame_number + 1;

  m_highest_frame_number = std::max(m_highest_frame_number, frame_number);
  auto new_entry         = fmt::format("{0}{1}", frame_number, entry.substr(comma_pos));
  packet->data           = memory_c::clone(new_entry);

  mxdebug_if(s_debug, fmt::format("fix_frame_number: from '{0}' to '{1}'\n", entry, new_entry));
}

connection_result_e
ssa_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                 std::string &error_message) {
  auto *psrc = dynamic_cast<ssa_packetizer_c *>(src);
  if (!psrc)
    return CAN_CONNECT_NO_FORMAT;

  connect_check_codec_private(src);

  return CAN_CONNECT_YES;
}

void
ssa_packetizer_c::connect(generic_packetizer_c *src,
                          int64_t append_timestamp_offset) {
  generic_packetizer_c::connect(src, append_timestamp_offset);

  if (1 == m_connected_to)
    m_preceding_packetizer = static_cast<ssa_packetizer_c *>(src);
}
