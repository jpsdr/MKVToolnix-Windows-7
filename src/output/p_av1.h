/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the AV1 output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/av1.h"
#include "merge/generic_packetizer.h"

class av1_video_packetizer_c: public generic_packetizer_c {
protected:
  int64_t m_previous_timestamp{-1};
  mtx::av1::parser_c m_parser;
  bool m_is_framed{true}, m_header_parameters_set{};

public:
  av1_video_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, unsigned int width, unsigned int height);

  virtual void set_is_unframed();

  virtual translatable_string_c get_format_name() const override {
    return YT("AV1");
  }

  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message) override;
  virtual bool is_compatible_with(output_compatibility_e compatibility) override;

protected:
  virtual void flush_impl() override;
  virtual void flush_frames();

  virtual void process_impl(packet_cptr const &packet) override;
  virtual void process_framed(packet_cptr const &packet);
  virtual void process_unframed();

  virtual void set_header_parameters();
};
