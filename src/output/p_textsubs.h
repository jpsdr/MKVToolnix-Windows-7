/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definition for the simple text subtitle packetizer

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/strings/editing.h"
#include "merge/generic_packetizer.h"

class textsubs_packetizer_c: public generic_packetizer_c {
protected:
  unsigned int m_packetno{};
  boost::optional<unsigned int> m_force_rerender_track_headers_on_packetno;
  charset_converter_cptr m_cc_utf8;
  std::string m_codec_id;
  line_ending_style_e m_line_ending_style{line_ending_style_e::cr_lf};

public:
  textsubs_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, const char *codec_id, bool recode, bool is_utf8);
  virtual ~textsubs_packetizer_c();

  virtual int process(packet_cptr packet);
  virtual void set_headers();
  virtual void set_line_ending_style(line_ending_style_e line_ending_style);

  virtual translatable_string_c get_format_name() const {
    return YT("text subtitles");
  }
  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message);
};
