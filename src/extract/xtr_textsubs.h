/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   extracts tracks from Matroska files into other files

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/xml/xml.h"
#include "extract/xtr_base.h"

class xtr_srt_c: public xtr_base_c {
public:
  int m_num_entries;
  std::string m_sub_charset;
  charset_converter_cptr m_conv;

  struct {
    int64_t m_timestamp{}, m_duration{};
    std::string m_text;
  } m_entry;

public:
  xtr_srt_c(const std::string &codec_id, int64_t tid, track_spec_t &tspec);

  virtual void create_file(xtr_base_c *master, libmatroska::KaxTrackEntry &track);
  virtual void handle_frame(xtr_frame_t &f);

  virtual const char *get_container_name() {
    return "SRT text subtitles";
  };

  virtual void flush_entry();
};

class xtr_ssa_c: public xtr_base_c {
public:
  class ssa_line_c {
  public:
    std::string m_line;
    int m_num;

    bool operator < (const ssa_line_c &cmp) const {
      return m_num < cmp.m_num;
    }

    ssa_line_c(std::string line, int num)
      : m_line(line)
      , m_num(num)
    { }
  };

  std::vector<std::string> m_ssa_format;
  std::vector<ssa_line_c> m_lines;
  std::string m_sub_charset, m_priv_post_events;
  charset_converter_cptr m_conv;
  bool m_warning_printed;
  unsigned int m_num_fields;

public:
  xtr_ssa_c(const std::string &codec_id, int64_t tid, track_spec_t &tspec);

  virtual void create_file(xtr_base_c *master, libmatroska::KaxTrackEntry &track);
  virtual void handle_frame(xtr_frame_t &f);
  virtual void finish_file();

  virtual const char *get_container_name() {
    return "SSA/ASS text subtitles";
  };
};

class xtr_usf_c: public xtr_base_c {
private:
  struct usf_entry_t {
    std::string m_text;
    int64_t m_start, m_end;

    usf_entry_t(const std::string &text, int64_t start, int64_t end):
      m_text(text), m_start(start), m_end(end) { }
  };

  std::string m_sub_charset, m_simplified_sub_charset, m_codec_private;
  mtx::bcp47::language_c m_language;
  mtx::xml::document_cptr m_doc;
  std::vector<usf_entry_t> m_entries;

public:
  xtr_usf_c(const std::string &codec_id, int64_t tid, track_spec_t &tspec);

  virtual void create_file(xtr_base_c *master, libmatroska::KaxTrackEntry &track);
  virtual void handle_frame(xtr_frame_t &f);
  virtual void finish_track();
  virtual void finish_file();

  virtual const char *get_container_name() {
    return "XML (USF text subtitles)";
  };
};
