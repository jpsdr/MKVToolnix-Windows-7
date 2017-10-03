/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   chapter parser/writer functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include <ebml/EbmlElement.h>

#include "common/timestamp.h"

namespace libebml {
  class EbmlMaster;
};

namespace libmatroska {
  class KaxChapters;
  class KaxTags;
  class KaxEditionEntry;
  class KaxChapterAtom;
};

using namespace libebml;
using namespace libmatroska;

class mm_io_c;
class mm_text_io_c;

namespace mtx { namespace chapters {

using kax_cptr = std::shared_ptr<KaxChapters>;

class parser_x: public exception {
protected:
  std::string m_message;
public:
  parser_x(const std::string &message)  : m_message(message)       { }
  parser_x(const boost::format &message): m_message(message.str()) { }
  virtual ~parser_x() throw() { }

  virtual const char *what() const throw() {
    return m_message.c_str();
  }
};

enum class format_e {
  xml,
  ogg,
  cue,
};

mtx::chapters::kax_cptr
parse(const std::string &file_name, int64_t min_ts = 0, int64_t max_ts = -1, int64_t offset = 0, const std::string &language = "", const std::string &charset = "",
      bool exception_on_error = false, format_e *format = nullptr, std::unique_ptr<KaxTags> *tags = nullptr);

mtx::chapters::kax_cptr
parse(mm_text_io_c *io, int64_t min_ts = 0, int64_t max_ts = -1, int64_t offset = 0, const std::string &language = "", const std::string &charset = "",
      bool exception_on_error = false, format_e *format = nullptr, std::unique_ptr<KaxTags> *tags = nullptr);

bool probe_simple(mm_text_io_c *in);
mtx::chapters::kax_cptr parse_simple(mm_text_io_c *in, int64_t min_ts, int64_t max_ts, int64_t offset, const std::string &language, const std::string &charset);

extern std::string g_cue_name_format, g_default_language, g_default_country;

bool probe_cue(mm_text_io_c *in);
mtx::chapters::kax_cptr parse_cue(mm_text_io_c *in, int64_t min_ts, int64_t max_ts, int64_t offset, const std::string &language, const std::string &charset, std::unique_ptr<KaxTags> *tags = nullptr);

std::size_t write_simple(KaxChapters &chapters, mm_io_c &out, boost::optional<std::string> const &language_to_extract);

bool select_in_timeframe(KaxChapters *chapters, int64_t min_ts, int64_t max_ts, int64_t offset);

int64_t get_uid(KaxChapterAtom &atom);
int64_t get_start(KaxChapterAtom &atom, int64_t value_if_not_found = -1);
int64_t get_end(KaxChapterAtom &atom, int64_t value_if_not_found = -1);
std::string get_name(KaxChapterAtom &atom);

void remove_elements_unsupported_by_webm(EbmlMaster &master);

KaxEditionEntry *find_edition_with_uid(KaxChapters &chapters, uint64_t uid);
KaxChapterAtom *find_chapter_with_uid(KaxChapters &chapters, uint64_t uid);

void move_by_edition(KaxChapters &dst, KaxChapters &src);
void adjust_timestamps(EbmlMaster &master, int64_t offset);
void merge_entries(EbmlMaster &master);
int count_atoms(EbmlMaster &master);
void regenerate_uids(EbmlMaster &master);

void align_uids(KaxChapters *chapters);
void align_uids(KaxChapters &reference, KaxChapters &modify);

std::string format_name_template(std::string const &name_template, int chapter_number, timestamp_c const &start_timestamp, std::string const &appended_file_name = std::string{});

void fix_country_codes(EbmlMaster &chapters);

}}
