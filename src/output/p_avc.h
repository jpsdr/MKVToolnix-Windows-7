/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the MPEG4 part 10 video output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "output/p_generic_video.h"

class avc_video_packetizer_c: public generic_video_packetizer_c {
protected:
  int m_nalu_size_len{};
  int64_t m_max_nalu_size{}, m_track_default_duration{-1};
  debugging_option_c m_debug_fix_bistream_timing_info;

public:
  avc_video_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, int64_t default_duration, int width, int height);
  virtual void set_headers();

  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message);

  virtual translatable_string_c get_format_name() const {
    return YT("AVC/H.264");
  }

protected:
  virtual void extract_aspect_ratio();
  virtual void process_impl(packet_cptr const &packet) override;
  virtual void process_nalus(memory_c &data) const;
};
