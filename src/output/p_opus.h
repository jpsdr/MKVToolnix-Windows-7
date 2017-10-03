/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definition for the Opus packetizer

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/opus.h"
#include "merge/generic_packetizer.h"

class opus_packetizer_c: public generic_packetizer_c {
private:
  static bool ms_experimental_warning_shown;

  debugging_option_c m_debug;
  timestamp_c m_next_calculated_timestamp, m_previous_provided_timestamp;
  mtx::opus::id_header_t m_id_header;

public:
  opus_packetizer_c(generic_reader_c *reader,  track_info_c &ti);
  virtual ~opus_packetizer_c();

  virtual int process(packet_cptr packet);
  virtual void set_headers();

  virtual translatable_string_c get_format_name() const {
    return YT("Opus");
  }
  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message);

  virtual bool is_compatible_with(output_compatibility_e compatibility);
};
