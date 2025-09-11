/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   chapter parser/writer functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include <ebml/EbmlElement.h>

#include "common/bcp47.h"
#include "common/math.h"
#include "common/timestamp.h"
#include "common/translation.h"

namespace libebml {
class EbmlMaster;
}

namespace libmatroska {
class KaxChapterAtom;
class KaxChapterDisplay;
class KaxChapters;
class KaxEditionEntry;
class KaxTags;
}

namespace mtx::chapters {

using kax_cptr = std::shared_ptr<libmatroska::KaxChapters>;

class parser_x: public exception {
protected:
  std::string m_message;
public:
  parser_x(const std::string &message) : m_message{message} { }
  virtual ~parser_x() throw() { }

  virtual const char *what() const throw() {
    return m_message.c_str();
  }
};

enum class format_e {
  xml,
  ogg,
  cue,
  ffmpeg_meta,
  dvd,
  bluray,
  potplayer_bookmarks,
};

mtx::chapters::kax_cptr
parse(const std::string &file_name, int64_t min_ts = 0, int64_t max_ts = -1, int64_t offset = 0, mtx::bcp47::language_c const &language = {}, const std::string &charset = "",
      bool exception_on_error = false, format_e *format = nullptr, std::unique_ptr<libmatroska::KaxTags> *tags = nullptr);

mtx::chapters::kax_cptr
parse(mm_text_io_c *io, int64_t min_ts = 0, int64_t max_ts = -1, int64_t offset = 0, mtx::bcp47::language_c const &language = {}, const std::string &charset = "",
      bool exception_on_error = false, format_e *format = nullptr, std::unique_ptr<libmatroska::KaxTags> *tags = nullptr);

bool probe_simple(mm_text_io_c *in);
mtx::chapters::kax_cptr parse_simple(mm_text_io_c *in, int64_t min_ts, int64_t max_ts, int64_t offset, mtx::bcp47::language_c const &language, const std::string &charset);

extern std::string g_cue_name_format, g_default_country;
extern mtx::bcp47::language_c g_default_language;
extern translatable_string_c g_chapter_generation_name_template;

bool probe_cue(mm_text_io_c *in);
mtx::chapters::kax_cptr parse_cue(mm_text_io_c *in, int64_t min_ts, int64_t max_ts, int64_t offset, mtx::bcp47::language_c const &language, const std::string &charset, std::unique_ptr<libmatroska::KaxTags> *tags = nullptr);

std::size_t write_simple(libmatroska::KaxChapters &chapters, mm_io_c &out, mtx::bcp47::language_c const &language_to_extract);

bool select_in_timeframe(libmatroska::KaxChapters *chapters, int64_t min_ts, int64_t max_ts, int64_t offset);

int64_t get_uid(libmatroska::KaxChapterAtom &atom);
int64_t get_start(libmatroska::KaxChapterAtom &atom, int64_t value_if_not_found = -1);
int64_t get_end(libmatroska::KaxChapterAtom &atom, int64_t value_if_not_found = -1);
std::string get_name(libmatroska::KaxChapterAtom &atom);

void remove_elements_unsupported_by_webm(libebml::EbmlMaster &master);

libmatroska::KaxEditionEntry *find_edition_with_uid(libmatroska::KaxChapters &chapters, uint64_t uid);
libmatroska::KaxChapterAtom *find_chapter_with_uid(libmatroska::KaxChapters &chapters, uint64_t uid);

void move_by_edition(libmatroska::KaxChapters &dst, libmatroska::KaxChapters &src);
void adjust_timestamps(libebml::EbmlMaster &master, int64_t offset, mtx_mp_rational_t const &factor = 1);
void merge_entries(libebml::EbmlMaster &master);
int count_atoms(libebml::EbmlMaster &master);
void regenerate_uids(libebml::EbmlMaster &master, libebml::EbmlMaster *tags = nullptr);

void align_uids(libmatroska::KaxChapters *chapters);
void align_uids(libmatroska::KaxChapters &reference, libmatroska::KaxChapters &modify);

std::string format_name_template(std::string const &name_template, int chapter_number, timestamp_c const &start_timestamp, std::string const &appended_file_name = std::string{}, std::string const &appended_title = std::string{});

void fix_country_codes(libebml::EbmlMaster &chapters);

std::shared_ptr<libmatroska::KaxChapters> create_editions_and_chapters(std::vector<std::vector<timestamp_c>> const &editions_timestamps, mtx::bcp47::language_c const &language, std::string const &name_template);

void set_languages_in_display(libmatroska::KaxChapterDisplay &display, mtx::bcp47::language_c const &parsed_language);
void set_languages_in_display(libmatroska::KaxChapterDisplay &display, std::string const &language);
void set_languages_in_display(libmatroska::KaxChapterDisplay &display, std::vector<mtx::bcp47::language_c> const &parsed_languages);
mtx::bcp47::language_c get_language_from_display(libmatroska::KaxChapterDisplay &display, std::string const &default_if_missing);

void unify_legacy_and_bcp47_languages_and_countries(libebml::EbmlElement &elt);

}
