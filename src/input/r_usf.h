/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definition for the USF subtitle reader

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "merge/generic_reader.h"

struct usf_entry_t {
  int64_t m_start, m_end;
  std::string m_text;

  usf_entry_t()
    : m_start(-1)
    , m_end(-1)
  {
  }

  usf_entry_t(int64_t start,
              int64_t end,
              std::string const &text)
    : m_start(start)
    , m_end(end)
    , m_text(text)
  {
  }

  bool operator <(const usf_entry_t &cmp) const {
    return m_start < cmp.m_start;
  }
};

struct usf_track_t {
  int m_ptzr{-1};

  mtx::bcp47::language_c m_language;
  std::vector<usf_entry_t> m_entries;
  std::vector<usf_entry_t>::const_iterator m_current_entry;
  int64_t m_byte_size{};
};
using usf_track_cptr = std::shared_ptr<usf_track_t>;

class usf_reader_c: public generic_reader_c {
private:
  std::vector<usf_track_cptr> m_tracks;
  std::string m_private_data;
  mtx::bcp47::language_c m_default_language;
  usf_track_cptr m_longest_track;
  int64_t m_bytes_to_process{}, m_bytes_processed{};

public:
  virtual mtx::file_type_e get_format_type() const {
    return mtx::file_type_e::usf;
  }

  virtual void read_headers();
  virtual void identify();
  virtual void create_packetizer(int64_t tid);
  virtual void create_packetizers();
  virtual int64_t get_progress() override;
  virtual int64_t get_maximum_progress() override;
  virtual bool is_simple_subtitle_container() {
    return true;
  }

  virtual bool probe_file() override;

protected:
  virtual file_status_e read(generic_packetizer_c *packetizer, bool force = false) override;

  virtual int64_t try_to_parse_timestamp(const char *s);
  virtual void parse_metadata(mtx::xml::document_cptr &doc);
  virtual void parse_subtitles(mtx::xml::document_cptr &doc);
  virtual void create_codec_private(mtx::xml::document_cptr &doc);

};
