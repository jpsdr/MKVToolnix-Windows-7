/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the VPX output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/ivf.h"
#include "merge/generic_packetizer.h"

class vpx_video_packetizer_c: public generic_packetizer_c {
protected:
  int64_t m_previous_timestamp;
  codec_c::type_e m_codec;
  bool m_is_vp9{};

public:
  vpx_video_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, codec_c::type_e p_codec);

  virtual void set_headers();

  virtual translatable_string_c get_format_name() const {
    return YT("VP8/VP9");
  }

  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message);
  virtual bool is_compatible_with(output_compatibility_e compatibility);

protected:
  virtual void process_impl(packet_cptr const &packet) override;
  virtual void vp9_determine_codec_private(memory_c const &mem);
};
