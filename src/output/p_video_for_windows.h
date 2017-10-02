/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definition for the Video for Windows output module

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "output/p_generic_video.h"

class video_for_windows_packetizer_c: public generic_video_packetizer_c {
protected:
  bool m_rederive_frame_types;

  enum {
    ct_unknown,
    ct_div3,
    ct_mpeg4_p2,
  } m_codec_type;

public:
  video_for_windows_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, double fps, int width, int height);

  virtual int process(packet_cptr packet) override;
  virtual void set_headers() override;

  virtual translatable_string_c get_format_name() const override {
    return YT("VfW compatible video");
  }

protected:
  virtual void check_fourcc();
  virtual void rederive_frame_type(packet_cptr &packet);
  virtual void rederive_frame_type_div3(packet_cptr &packet);
  virtual void rederive_frame_type_mpeg4_p2(packet_cptr &packet);
};
