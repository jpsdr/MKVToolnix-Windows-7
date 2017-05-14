/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   chapter parser/writer functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#ifndef MTX_COMMON_CHAPTERS_H
#define MTX_COMMON_CHAPTERS_H

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

namespace mtx {
  class chapter_parser_x: public exception {
  protected:
    std::string m_message;
  public:
    chapter_parser_x(const std::string &message)  : m_message(message)       { }
    chapter_parser_x(const boost::format &message): m_message(message.str()) { }
    virtual ~chapter_parser_x() throw() { }

    virtual const char *what() const throw() {
      return m_message.c_str();
    }
  };
}

class mm_io_c;
class mm_text_io_c;

using kax_chapters_cptr = std::shared_ptr<KaxChapters>;

enum class chapter_format_e {
  xml,
  ogg,
  cue,
};

kax_chapters_cptr
parse_chapters(const std::string &file_name, int64_t min_tc = 0, int64_t max_tc = -1, int64_t offset = 0, const std::string &language = "", const std::string &charset = "",
               bool exception_on_error = false, chapter_format_e *format = nullptr, std::unique_ptr<KaxTags> *tags = nullptr);

kax_chapters_cptr
parse_chapters(mm_text_io_c *io, int64_t min_tc = 0, int64_t max_tc = -1, int64_t offset = 0, const std::string &language = "", const std::string &charset = "",
               bool exception_on_error = false, chapter_format_e *format = nullptr, std::unique_ptr<KaxTags> *tags = nullptr);

bool probe_simple_chapters(mm_text_io_c *in);
kax_chapters_cptr parse_simple_chapters(mm_text_io_c *in, int64_t min_tc, int64_t max_tc, int64_t offset, const std::string &language, const std::string &charset);

extern std::string g_cue_to_chapter_name_format;
bool probe_cue_chapters(mm_text_io_c *in);
kax_chapters_cptr parse_cue_chapters(mm_text_io_c *in, int64_t min_tc, int64_t max_tc, int64_t offset, const std::string &language, const std::string &charset, std::unique_ptr<KaxTags> *tags = nullptr);

std::size_t write_chapters_simple(KaxChapters &chapters, mm_io_c &out, boost::optional<std::string> const &language_to_extract);

bool select_chapters_in_timeframe(KaxChapters *chapters, int64_t min_tc, int64_t max_tc, int64_t offset);

extern std::string g_default_chapter_language, g_default_chapter_country;

int64_t get_chapter_uid(KaxChapterAtom &atom);
int64_t get_chapter_start(KaxChapterAtom &atom, int64_t value_if_not_found = -1);
int64_t get_chapter_end(KaxChapterAtom &atom, int64_t value_if_not_found = -1);
std::string get_chapter_name(KaxChapterAtom &atom);

void fix_mandatory_chapter_elements(EbmlElement *e);
KaxEditionEntry *find_edition_with_uid(KaxChapters &chapters, uint64_t uid);
KaxChapterAtom *find_chapter_with_uid(KaxChapters &chapters, uint64_t uid);

void move_chapters_by_edition(KaxChapters &dst, KaxChapters &src);
void adjust_chapter_timecodes(EbmlMaster &master, int64_t offset);
void merge_chapter_entries(EbmlMaster &master);
int count_chapter_atoms(EbmlMaster &master);
void regenerate_edition_and_chapter_uids(EbmlMaster &master);

void align_chapter_edition_uids(KaxChapters *chapters);
void align_chapter_edition_uids(KaxChapters &reference, KaxChapters &modify);

std::string format_chapter_name_template(std::string const &name_template, int chapter_number, timestamp_c const &start_timestamp, std::string const &appended_file_name = std::string{});

void fix_chapter_country_codes(EbmlMaster &chapters);

#endif // MTX_COMMON_CHAPTERS_H
