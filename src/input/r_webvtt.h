/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the WebVTT subtitle reader

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/webvtt.h"
#include "merge/generic_reader.h"

class webvtt_reader_c: public generic_reader_c {
private:
  mtx::webvtt::parser_cptr m_parser;
  int64_t m_bytes_to_process{}, m_bytes_processed{};

public:
  virtual mtx::file_type_e get_format_type() const {
    return mtx::file_type_e::webvtt;
  }

  virtual void read_headers();
  virtual void identify();
  virtual void create_packetizer(int64_t tid);
  virtual int64_t get_progress() override;
  virtual int64_t get_maximum_progress() override;
  virtual bool is_simple_subtitle_container() {
    return true;
  }

  virtual bool probe_file() override;

protected:
  virtual file_status_e read(generic_packetizer_c *packetizer, bool force = false) override;

  virtual void parse_file();
};
