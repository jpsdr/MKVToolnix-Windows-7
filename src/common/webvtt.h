/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   definitions and helper functions for WebVTT data

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/timestamp.h"

namespace mtx::webvtt {

class parser_c {
public:
  struct cue_t {
    timestamp_c m_start, m_duration;
    memory_cptr m_content, m_addition;
  };
  using cue_cptr = std::shared_ptr<cue_t>;

private:
  struct impl_t;
  std::unique_ptr<impl_t> m;

public:
  parser_c();
  ~parser_c();

  void add_line(std::string const &line);
  void add_joined_lines(std::string const &joined_lines);
  void add_joined_lines(memory_c const &mem);
  void flush();

  bool codec_private_available() const;
  memory_cptr get_codec_private() const;

  bool cue_available() const;
  cue_cptr get_cue();

  unsigned int get_current_cue_number() const;
  unsigned int get_total_number_of_cues() const;
  unsigned int get_total_number_of_bytes() const;

protected:
  void add_block();

public:
  static std::string adjust_embedded_timestamps(std::string const &text, timestamp_c const &offset);
};
using parser_cptr = std::shared_ptr<parser_c>;

} // namespace mtx::webvtt
