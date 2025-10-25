/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   chapter parser for cue sheets

   Written by Moritz Bunkus <mo@bunkus.online>.
   Patches by Nicolas Le Guen <nleguen@pepper-prod.com> and
   Vegard Pettersen <vegard_p@broadpark.no>
*/

#include "common/common_pch.h"

#include "common/chapters/chapters.h"
#include "common/chapters/physical.h"
#include "common/ebml.h"
#include "common/error.h"
#include "common/locale.h"
#include "common/mm_proxy_io.h"
#include "common/mm_text_io.h"
#include "common/strings/editing.h"
#include "common/strings/formatting.h"
#include "common/strings/utf8.h"
#include "common/tags/target_type.h"
#include "common/unique_numbers.h"

namespace mtx::chapters {

// PERFORMER "Blackmore's Night"
// TITLE "Fires At Midnight"
// FILE "Range.wav" WAVE
//   TRACK 01 AUDIO
//     TITLE "Written In The Stars"
//     PERFORMER "Blackmore's Night"
//     INDEX 01 00:00:00
//   TRACK 02 AUDIO
//     TITLE "The Times They Are A Changin'"
//     PERFORMER "Blackmore's Night"
//     INDEX 00 04:46:62
//     INDEX 01 04:49:64

bool
probe_cue(mm_text_io_c *in) {
  std::string s;

  in->setFilePointer(0);
  if (!in->getline2(s))
    return false;

  return (balg::istarts_with(s, "performer ") || balg::istarts_with(s, "title ") || balg::istarts_with(s, "file ") || balg::istarts_with(s, "catalog ") || balg::istarts_with(s, "rem "));
}

std::string g_cue_name_format;

static void
cue_entries_to_name(std::string &performer,
                    std::string &title,
                    std::string &global_performer,
                    std::string &global_title,
                    std::string &name,
                    int num) {
  name = "";

  if (title.empty())
    title = global_title;

  if (performer.empty())
    performer = global_performer;

  const char *this_char = mtx::chapters::g_cue_name_format.empty() ? "%p - %t" : mtx::chapters::g_cue_name_format.c_str();
  const char *next_char = this_char + 1;

  while (0 != *this_char) {
    if (*this_char == '%') {
      if (*next_char == 'p')
        name += performer;
      else if (*next_char == 't')
        name += title;
      else if (*next_char == 'n')
        name += fmt::to_string(num);
      else if (*next_char == 'N') {
        if (num < 10)
          name += '0';
        name += fmt::to_string(num);
      } else {
        name += *this_char;
        this_char--;
      }
      this_char++;
    } else
      name += *this_char;
    this_char++;
    next_char = this_char + 1;
  }
}

struct cue_parser_args_t {
  int num{};
  int64_t start_of_track{-1};
  std::vector<int64_t> start_indices;
  bool index00_missing{};
  int64_t end{};
  int64_t min_ts{};
  int64_t max_ts{};
  int64_t offset{};
  libmatroska::KaxChapters *chapters{};
  libmatroska::KaxEditionEntry *edition{};
  libmatroska::KaxChapterAtom *atom{};
  bool do_convert{};
  std::string global_catalog;
  std::string global_performer;
  std::string performer;
  std::string global_title;
  std::string title;
  std::string name;
  std::string global_date;
  std::string date;
  std::string global_genre;
  std::string genre;
  std::string global_disc_id;
  std::string isrc;
  std::string flags;
  std::vector<std::string> global_rem;
  std::vector<std::string> global_comment;
  std::vector<std::string> comment;
  mtx::bcp47::language_c language;
  int line_num{};
  charset_converter_cptr cc_utf8;
};

static libebml::UTFstring
cue_str_internal_to_utf(cue_parser_args_t &a,
                        const std::string &s) {
  return to_utfstring(a.do_convert ? a.cc_utf8->utf8(s) : s);
}

static libmatroska::KaxTagSimple *
create_simple_tag(cue_parser_args_t &a,
                  const std::string &name,
                  const std::string &value) {
  auto simple = new libmatroska::KaxTagSimple;

  get_child<libmatroska::KaxTagName>(*simple).SetValue(cue_str_internal_to_utf(a, name));
  get_child<libmatroska::KaxTagString>(*simple).SetValue(cue_str_internal_to_utf(a, value));

  return simple;
}

static void
create_tag1(cue_parser_args_t &a,
            libmatroska::KaxTag &tag,
            std::string const &v1,
            char const *text) {
  if (!v1.empty())
    tag.PushElement(*create_simple_tag(a, text, v1));
}

static void
create_tag2(cue_parser_args_t &a,
            libmatroska::KaxTag &tag,
            std::string const &v1,
            std::string const &v2,
            char const *text) {
  auto const &v = v1.empty() ? v2 : v1;
  if (!v.empty())
    tag.PushElement(*create_simple_tag(a, text, v));
}

static void
add_tag_for_cue_entry(cue_parser_args_t &a,
                      std::unique_ptr<libmatroska::KaxTags> *tags,
                      uint64_t cuid) {
  if (!tags)
    return;

  if (!*tags)
    *tags = std::make_unique<libmatroska::KaxTags>();

  auto tag      = new libmatroska::KaxTag;
  auto &targets = get_child<libmatroska::KaxTagTargets>(*tag);

  get_child<libmatroska::KaxTagChapterUID>(targets).SetValue(cuid);
  get_child<libmatroska::KaxTagTargetTypeValue>(targets).SetValue(mtx::tags::Track);
  get_child<libmatroska::KaxTagTargetType>(targets).SetValue("track");

  create_tag1(a, *tag, a.title, "TITLE");
  tag->PushElement(*create_simple_tag(a, "PART_NUMBER", fmt::to_string(a.num)));
  create_tag2(a, *tag, a.performer, a.global_performer, "ARTIST");
  create_tag2(a, *tag, a.date, a.global_date, "DATE_RELEASED");
  create_tag2(a, *tag, a.genre, a.global_genre, "GENRE");
  create_tag1(a, *tag, a.isrc, "ISRC");
  create_tag1(a, *tag, a.flags, "CDAUDIO_TRACK_FLAGS");

  size_t i;
  for (i = 0; i < a.global_comment.size(); i++)
    create_tag1(a, *tag, a.global_comment[i], "COMMENT");

  for (i = 0; i < a.comment.size(); i++)
    create_tag1(a, *tag, a.comment[i], "COMMENT");

  if (find_child<libmatroska::KaxTagSimple>(tag))
    (*tags)->PushElement(*tag);
  else
    delete tag;
}

static void
add_tag_for_global_cue_settings(cue_parser_args_t &a,
                                std::unique_ptr<libmatroska::KaxTags> *tags) {
  if (!tags)
    return;

  if (!*tags)
    *tags = std::make_unique<libmatroska::KaxTags>();

  auto tag      = new libmatroska::KaxTag;
  auto &targets = get_child<libmatroska::KaxTagTargets>(*tag);

  get_child<libmatroska::KaxTagTargetTypeValue>(targets).SetValue(mtx::tags::Album);
  get_child<libmatroska::KaxTagTargetType>(targets).SetValue("ALBUM");

  create_tag1(a, *tag, a.global_performer, "ARTIST");
  create_tag1(a, *tag, a.global_title,     "TITLE");
  create_tag1(a, *tag, a.global_date,      "DATE_RELEASED");
  create_tag1(a, *tag, a.global_disc_id,   "DISCID");
  create_tag1(a, *tag, a.global_catalog,   "CATALOG_NUMBER");

  size_t i;
  for (i = 0; i < a.global_rem.size(); i++)
    create_tag1(a, *tag, a.global_rem[i], "COMMENT");

  if (find_child<libmatroska::KaxTagSimple>(tag))
    (*tags)->PushElement(*tag);
  else
    delete tag;
}

static void
add_subchapters_for_index_entries(cue_parser_args_t &a) {
  if (a.start_indices.empty())
    return;

  libmatroska::KaxChapterAtom *atom = nullptr;
  size_t offset        = a.index00_missing ? 1 : 0;
  size_t i;
  for (i = 0; i < a.start_indices.size(); i++) {
    atom = &get_first_or_next_child<libmatroska::KaxChapterAtom>(*a.atom, atom);

    get_child<libmatroska::KaxChapterUID>(*atom).SetValue(create_unique_number(UNIQUE_CHAPTER_IDS));
    get_child<libmatroska::KaxChapterTimeStart>(*atom).SetValue(a.start_indices[i] - a.offset);
    get_child<libmatroska::KaxChapterFlagHidden>(*atom).SetValue(1);
    get_child<libmatroska::KaxChapterPhysicalEquiv>(*atom).SetValue(mtx::chapters::PHYSEQUIV_INDEX);

    auto &display = get_child<libmatroska::KaxChapterDisplay>(*atom);
    get_child<libmatroska::KaxChapterString>(display).SetValueUTF8(fmt::format("INDEX {0:02}", i + offset));
    get_child<libmatroska::KaxChapterLanguage>(display).SetValue("eng");
  }
}

static void
add_elements_for_cue_entry(cue_parser_args_t &a,
                           std::unique_ptr<libmatroska::KaxTags> *tags) {
  if (a.start_indices.empty())
    mxerror(fmt::format(FY("Cue sheet parser: No INDEX entry found for the previous TRACK entry (current line: {0})\n"), a.line_num));

  if (!((a.start_indices[0] >= a.min_ts) && ((a.start_indices[0] <= a.max_ts) || (a.max_ts == -1))))
    return;

  mtx::chapters::cue_entries_to_name(a.performer, a.title, a.global_performer, a.global_title, a.name, a.num);

  if (!a.edition) {
    a.edition = &get_child<libmatroska::KaxEditionEntry>(*a.chapters);
    get_child<libmatroska::KaxEditionUID>(*a.edition).SetValue(create_unique_number(UNIQUE_EDITION_IDS));
  }

  a.atom = &get_first_or_next_child<libmatroska::KaxChapterAtom>(*a.edition, a.atom);
  get_child<libmatroska::KaxChapterPhysicalEquiv>(*a.atom).SetValue(mtx::chapters::PHYSEQUIV_TRACK);

  auto cuid = create_unique_number(UNIQUE_CHAPTER_IDS);
  get_child<libmatroska::KaxChapterUID>(*a.atom).SetValue(cuid);
  get_child<libmatroska::KaxChapterTimeStart>(*a.atom).SetValue(a.start_of_track - a.offset);

  auto &display = get_child<libmatroska::KaxChapterDisplay>(*a.atom);
  get_child<libmatroska::KaxChapterString>(display).SetValue(cue_str_internal_to_utf(a, a.name));
  get_child<libmatroska::KaxChapterLanguage>(display).SetValue(a.language.get_closest_iso639_2_alpha_3_code());

  add_subchapters_for_index_entries(a);

  add_tag_for_cue_entry(a, tags, cuid);
}

static std::string
get_quoted(std::string src,
           int offset) {
  src.erase(0, offset);
  mtx::string::strip(src);

  if (!src.empty() && (src[0] == '"'))
    src.erase(0, 1);

  if (!src.empty() && (src[src.length() - 1] == '"'))
    src.erase(src.length() - 1);

  return src;
}

static std::string
erase_colon(std::string &s,
            size_t skip) {
  size_t i = skip + 1;

  while ((s.length() > i) && (s[i] == ' '))
    i++;

  while ((s.length() > i) && (isalpha(s[i])))
    i++;

  if (s.length() == i)
    return s;

  if (':' == s[i])
    s.erase(i, 1);

  else if (s.substr(i, 2) == " :")
    s.erase(i, 2);

  return s;
}

mtx::chapters::kax_cptr
parse_cue(mm_text_io_c *in,
          int64_t min_ts,
          int64_t max_ts,
          int64_t offset,
          mtx::bcp47::language_c const &language,
          std::string const &charset,
          std::unique_ptr<libmatroska::KaxTags> *tags) {
  cue_parser_args_t a;
  std::string line;

  in->setFilePointer(0);
  auto chapters = std::make_shared<libmatroska::KaxChapters>();
  a.chapters    = chapters.get();

  if (in->get_byte_order_mark() == byte_order_mark_e::none) {
    a.do_convert = true;
    a.cc_utf8    = charset_converter_c::init(charset);
  }

  a.language = language.is_valid() ? language : mtx::bcp47::language_c::parse("eng");
  a.min_ts   = min_ts;
  a.max_ts   = max_ts;
  a.offset   = offset;

  while (in->getline2(line)) {
    a.line_num++;
    mtx::string::strip(line);

    if ((line.empty()) || balg::istarts_with(line, "file "))
      continue;

    if (balg::istarts_with(line, "performer ")) {
      if (0 == a.num)
        a.global_performer = get_quoted(line, 10);
      else
        a.performer        = get_quoted(line, 10);

    } else if (balg::istarts_with(line, "catalog "))
      a.global_catalog = get_quoted(line, 8);

    else if (balg::istarts_with(line, "title ")) {
      if (0 == a.num)
        a.global_title = get_quoted(line, 6);
      else
        a.title        = get_quoted(line, 6);

    } else if (balg::istarts_with(line, "index ")) {
      unsigned int index, min, sec, frames;

      line.erase(0, 6);
      mtx::string::strip(line);
      if (sscanf(line.c_str(), "%u %u:%u:%u", &index, &min, &sec, &frames) < 4)
        mxerror(fmt::format(FY("Cue sheet parser: Invalid INDEX entry in line {0}.\n"), a.line_num));

      bool index_ok = false;
      if (99 >= index) {
        if ((a.start_indices.empty()) && (1 == index))
          a.index00_missing = true;

        if ((a.start_indices.size() == index) || ((a.start_indices.size() == (index - 1)) && a.index00_missing)) {
          int64_t timestamp = min * 60 * 1000000000ll + sec * 1000000000ll + frames * 1000000000ll / 75;
          a.start_indices.push_back(timestamp);

          if ((1 == index) || (0 == index))
            a.start_of_track = timestamp;

          index_ok = true;
        }
      }

      if (!index_ok)
        mxerror(fmt::format(FY("Cue sheet parser: Invalid INDEX number (got {0}, expected {1}) in line {2}.\n"), index, a.start_indices.size(), a.line_num));

    } else if (balg::istarts_with(line, "track ")) {
      if ((line.length() < 5) || strcasecmp(&line[line.length() - 5], "audio"))
        continue;

      if (1 <= a.num)
        add_elements_for_cue_entry(a, tags);
      else
        add_tag_for_global_cue_settings(a, tags);

      a.num++;
      a.start_of_track  = -1;
      a.index00_missing = false;
      a.performer       = "";
      a.title           = "";
      a.isrc            = "";
      a.date            = "";
      a.genre           = "";
      a.flags           = "";
      a.start_indices.clear();
      a.comment.clear();

    } else if (balg::istarts_with(line, "isrc "))
      a.isrc = get_quoted(line, 5);

    else if (balg::istarts_with(line, "flags "))
      a.flags = get_quoted(line, 6);

    else if (balg::istarts_with(line, "rem ")) {
      erase_colon(line, 4);
      if (balg::istarts_with(line, "rem date ") || balg::istarts_with(line, "rem year ")) {
        if (0 == a.num)
          a.global_date = get_quoted(line, 9);
        else
          a.date        = get_quoted(line, 9);

      } else if (balg::istarts_with(line, "rem genre ")) {
        if (0 == a.num)
          a.global_genre = get_quoted(line, 10);
        else
          a.genre        = get_quoted(line, 10);

      } else if (balg::istarts_with(line, "rem discid "))
        a.global_disc_id = get_quoted(line, 11);

      else if (balg::istarts_with(line, "rem comment ")) {
        if (0 == a.num)
          a.global_comment.push_back(get_quoted(line, 12));
        else
          a.comment.push_back(get_quoted(line, 12));

      } else {
        if (0 == a.num)
          a.global_rem.push_back(get_quoted(line, 4));
        else
          a.comment.push_back(get_quoted(line, 4));
      }
    }
  }

  if (1 <= a.num)
    add_elements_for_cue_entry(a, tags);

  return 0 == a.num ? nullptr : chapters;
}

}
