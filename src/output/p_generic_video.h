/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definition for the generic video output module

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "merge/generic_packetizer.h"

#define VFT_IFRAME          -1
#define VFT_PFRAMEAUTOMATIC -2
#define VFT_NOBFRAME        -1

class generic_video_packetizer_c: public generic_packetizer_c {
protected:
  double m_fps;
  int m_width, m_height, m_frames_output;
  int64_t m_ref_timestamp, m_duration_shift;

public:
  generic_video_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, std::string const &codec_id, double fps, int width, int height);

  virtual int process(packet_cptr packet) override;
  virtual void set_headers() override;

  virtual translatable_string_c get_format_name() const override {
    return YT("generic video");
  }
  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message) override;
};
