/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the VC1 video output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "merge/generic_packetizer.h"
#include "common/vc1.h"

class vc1_video_packetizer_c: public generic_packetizer_c {
protected:
  mtx::vc1::es_parser_c m_parser;
  mtx::vc1::sequence_header_t m_seqhdr;
  memory_cptr m_raw_headers;

  int64_t m_previous_timestamp;

public:
  vc1_video_packetizer_c(generic_reader_c *n_reader, track_info_c &n_ti);

  virtual void set_headers();

  virtual translatable_string_c get_format_name() const {
    return YT("VC-1");
  };

  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message);

protected:
  virtual void flush_impl();
  virtual void flush_frames();
  virtual void headers_found();
  virtual void process_impl(packet_cptr const &packet) override;
  virtual void add_timestamps_to_parser(packet_cptr const &packet);
};
