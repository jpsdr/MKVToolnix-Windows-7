/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the video output module

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/mpeg1_2.h"
#include "output/p_generic_video.h"
#include "mpegparser/M2VParser.h"

class mpeg1_2_video_packetizer_c: public generic_video_packetizer_c {
protected:
  M2VParser m_parser;
  memory_cptr m_seq_hdr;
  bool m_framed, m_aspect_ratio_extracted;
  int64_t m_num_removed_stuffing_bytes;
  debugging_option_c m_debug_stuffing_removal;

public:
  mpeg1_2_video_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, int version, int64_t default_duration, int width, int height, int dwidth, int dheight, bool framed);
  virtual ~mpeg1_2_video_packetizer_c();

  virtual translatable_string_c get_format_name() const {
    return YT("MPEG-1/2 video");
  }

protected:
  virtual void process_impl(packet_cptr const &packet) override;
  virtual void extract_fps(const uint8_t *buffer, int size);
  virtual void extract_aspect_ratio(const uint8_t *buffer, int size);
  virtual void process_framed(packet_cptr const &packet);
  virtual void process_unframed(packet_cptr const &packet);
  virtual void remove_stuffing_bytes_and_handle_sequence_headers(packet_cptr const &packet);
  virtual void flush_impl();
};
