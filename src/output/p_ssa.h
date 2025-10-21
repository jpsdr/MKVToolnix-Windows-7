/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "output/p_textsubs.h"

class ssa_packetizer_c: public textsubs_packetizer_c {
protected:
  int m_highest_frame_number{-1};
  ssa_packetizer_c *m_preceding_packetizer{};

public:
  ssa_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, const char *codec_id, bool recode);
  virtual ~ssa_packetizer_c();

  virtual translatable_string_c get_format_name() const override {
    return YT("SSA/ASS text subtitles");
  }
  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message) override;
  virtual void connect(generic_packetizer_c *src, int64_t append_timestamp_offset) override;

  virtual void fix_frame_number(packet_cptr const &packet);

protected:
  virtual void process_impl(packet_cptr const &packet) override;
};
