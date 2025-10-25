/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the generic video output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "merge/generic_packetizer.h"

constexpr auto VFT_IFRAME          = -1;
constexpr auto VFT_PFRAMEAUTOMATIC = -2;
constexpr auto VFT_NOBFRAME        = -1;

class generic_video_packetizer_c: public generic_packetizer_c {
protected:
  int m_width, m_height, m_frames_output;
  int64_t m_default_duration,  m_ref_timestamp, m_duration_shift, m_bframe_bref;

public:
  generic_video_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, std::string const &codec_id, int64_t default_duration, int width, int height);

  virtual void set_headers() override;

  virtual translatable_string_c get_format_name() const override {
    return YT("generic video");
  }
  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message) override;

protected:
  virtual void process_impl(packet_cptr const &packet) override;
};
