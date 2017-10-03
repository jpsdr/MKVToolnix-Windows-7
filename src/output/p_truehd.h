/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definition for the TrueHD output module

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/truehd.h"
#include "merge/generic_packetizer.h"
#include "merge/timestamp_calculator.h"

class truehd_packetizer_c: public generic_packetizer_c {
protected:
  bool m_first_frame;
  truehd_frame_t m_first_truehd_header;

  int64_t m_current_samples_per_frame, m_ref_timestamp;
  timestamp_calculator_c m_timestamp_calculator;
  truehd_parser_c m_parser;

public:
  truehd_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, truehd_frame_t::codec_e codec, int sampling_rate, int channels);
  virtual ~truehd_packetizer_c();

  virtual int process(packet_cptr packet);
  virtual void process_framed(truehd_frame_cptr const &frame, int64_t provided_timestamp);
  virtual void set_headers();

  virtual translatable_string_c get_format_name() const {
    return YT("TrueHD");
  }

  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message);

protected:
  virtual void adjust_header_values(truehd_frame_cptr const &frame);

  virtual void flush_impl();
  virtual void flush_frames();
};
