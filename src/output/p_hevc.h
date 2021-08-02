/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the HEVC video output module

*/

#pragma once

#include "common/common_pch.h"

#include "output/p_generic_video.h"

class hevc_video_packetizer_private_c;
class hevc_video_packetizer_c: public generic_video_packetizer_c {
protected:
  MTX_DECLARE_PRIVATE(hevc_video_packetizer_private_c)

  std::unique_ptr<hevc_video_packetizer_private_c> const p_ptr;

  explicit hevc_video_packetizer_c(hevc_video_packetizer_private_c &p);

public:
  hevc_video_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, int64_t default_duration, int width, int height);
  virtual void set_headers() override;

  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message) override;

  virtual translatable_string_c get_format_name() const override {
    return YT("HEVC/H.265");
  }

  virtual void set_source_timestamp_resolution(int64_t resolution);

protected:
  virtual void process_impl(packet_cptr const &packet) override;
  virtual void connect(generic_packetizer_c *src, int64_t append_timestamp_offset = -1) override;

  virtual void extract_aspect_ratio();

  virtual void flush_impl() override;
  virtual void flush_frames();

  virtual void setup_default_duration();
};
