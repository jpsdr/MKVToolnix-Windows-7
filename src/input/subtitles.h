/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the subtitle helper

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "merge/output_control.h"
#include "output/p_textsubs.h"

struct sub_t {
  int64_t start, end;
  unsigned int number;
  std::string subs;

  sub_t(int64_t _start, int64_t _end, unsigned int _number, const std::string &_subs):
    start(_start), end(_end), number(_number), subs(_subs) {
  }

  bool operator < (const sub_t &cmp) const {
    return start < cmp.start;
  }
};

class subtitles_c {
protected:
  std::deque<sub_t> entries;
  std::deque<sub_t>::iterator current;
  charset_converter_cptr m_cc_utf8;
  bool m_try_utf8{}, m_invalid_utf8_warned{};
  std::string m_file_name;
  int64_t m_track_id{};
  unsigned int m_num_skipped{};
  debugging_option_c m_debug{"srt_parser|ssa_parser|subtitle_parser"};

public:
  subtitles_c(std::string const &file_name, int64_t track_id);
  void add_maybe(int64_t start, int64_t end, unsigned int number, const std::string &subs);
  void add(int64_t start, int64_t end, unsigned int number, const std::string &subs) {
    entries.push_back(sub_t(start, end, number, subs));
  }
  void reset() {
    current = entries.begin();
  }
  int get_num_entries() {
    return entries.size();
  }
  int get_num_processed() {
    return std::distance(entries.begin(), current);
  }
  void process(generic_packetizer_c *);
  void sort() {
    std::stable_sort(entries.begin(), entries.end());
    reset();
  }
  bool empty() {
    return current == entries.end();
  }

  int64_t get_total_byte_size() {
    return std::accumulate(entries.begin(), entries.end(), 0ull, [](int64_t num, auto const &entry) { return num + entry.subs.length(); });
  }

  int64_t get_next_byte_size() {
    if (empty() || (entries.end() == current))
      return 0;
    return current->subs.length();
  }

  void set_charset_converter(charset_converter_cptr const &cc_utf8);
  std::string recode(std::string const &s, uint32_t replacement_marker = 0xfffdu);
};
using subtitles_cptr = std::shared_ptr<subtitles_c>;

class srt_parser_c: public subtitles_c {
public:
  enum parser_state_e {
    STATE_INITIAL,
    STATE_SUBS,
    STATE_SUBS_OR_NUMBER,
    STATE_TIME,
  };

protected:
  mm_text_io_cptr m_io;
  bool m_coordinates_warning_shown;

public:
  srt_parser_c(mm_text_io_cptr const &io, const std::string &file_name, int64_t track_id);
  void parse();

public:
  static bool probe(mm_text_io_c &io);
};
using srt_parser_cptr = std::shared_ptr<srt_parser_c>;

class ssa_parser_c: public subtitles_c {
public:
  enum ssa_section_e {
    SSA_SECTION_NONE,
    SSA_SECTION_INFO,
    SSA_SECTION_V4STYLES,
    SSA_SECTION_EVENTS,
    SSA_SECTION_GRAPHICS,
    SSA_SECTION_FONTS
  };

protected:
  generic_reader_c &m_reader;
  mm_text_io_cptr m_io;
  std::vector<std::string> m_format;
  bool m_is_ass;
  std::string m_global;
  int64_t m_attachment_id;

public:
  std::vector<attachment_t> m_attachments;

public:
  ssa_parser_c(generic_reader_c &reader, mm_text_io_cptr const &io, const std::string &file_name, int64_t track_id);
  void parse();

  bool is_ass() {
    return m_is_ass;
  }

  std::string get_global() {
    return m_global;
  }

  void set_attachment_id_base(int64_t id) {
    m_attachment_id = id;
  }

public:
  static bool probe(mm_text_io_c &io);

protected:
  int64_t parse_time(std::string &time);
  std::string get_element(const char *index, std::vector<std::string> &fields);
  void add_attachment_maybe(std::string &name, std::string &data_uu, ssa_section_e section);
  void decode_chars(uint8_t const *in, uint8_t *out, size_t bytes_in);
};
using ssa_parser_cptr = std::shared_ptr<ssa_parser_c>;
