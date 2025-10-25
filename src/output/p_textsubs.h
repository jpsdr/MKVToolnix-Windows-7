/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the simple text subtitle packetizer

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/strings/editing.h"
#include "merge/generic_packetizer.h"

class textsubs_packetizer_c: public generic_packetizer_c {
protected:
  unsigned int m_packetno{};
  std::optional<unsigned int> m_force_rerender_track_headers_on_packetno;
  charset_converter_cptr m_cc_utf8;
  bool m_try_utf8{}, m_invalid_utf8_warned{}, m_converter_is_utf8{}, m_strip_whitespaces{};
  std::string m_codec_id;
  mtx::string::line_ending_style_e m_line_ending_style{mtx::string::line_ending_style_e::cr_lf};
  packet_cptr m_buffered_packet;

public:
  textsubs_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, const char *codec_id, bool recode = false);
  virtual ~textsubs_packetizer_c();

  virtual void set_headers();
  virtual void set_line_ending_style(mtx::string::line_ending_style_e line_ending_style);

  virtual translatable_string_c get_format_name() const {
    return YT("text subtitles");
  }
  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message);

protected:
  virtual void process_impl(packet_cptr const &packet) override;
  virtual void process_one_packet(packet_cptr const &packet);
  virtual std::string recode(std::string subs);
};
