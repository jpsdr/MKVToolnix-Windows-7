/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the Dirac video output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "merge/generic_packetizer.h"
#include "common/dirac.h"

class dirac_video_packetizer_c: public generic_packetizer_c {
protected:
  mtx::dirac::es_parser_c m_parser;

  mtx::dirac::sequence_header_t m_seqhdr;
  bool m_headers_found;

  int64_t m_previous_timestamp;

public:
  dirac_video_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti);

  virtual void set_headers();

  virtual translatable_string_c get_format_name() const {
    return YT("Dirac");
  };

  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message);

protected:
  virtual void process_impl(packet_cptr const &packet) override;
  virtual void flush_impl();
  virtual void flush_frames();
  virtual void headers_found();
};
