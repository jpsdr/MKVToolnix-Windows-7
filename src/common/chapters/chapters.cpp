/** \brief chapter parser and helper functions

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \file

   \author Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <algorithm>
#include <cassert>
#include <optional>

#include <QRegularExpression>

#include "common/bcp47.h"
#include "common/chapters/chapters.h"
#include "common/chapters/bluray.h"
#include "common/chapters/dvd.h"
#include "common/construct.h"
#include "common/container.h"
#include "common/debugging.h"
#include "common/ebml.h"
#include "common/error.h"
#include "common/iso3166.h"
#include "common/locale.h"
#include "common/math_fwd.h"
#include "common/mm_io_x.h"
#include "common/mm_file_io.h"
#include "common/mm_proxy_io.h"
#include "common/mm_text_io.h"
#include "common/path.h"
#include "common/qt.h"
#include "common/strings/editing.h"
#include "common/strings/formatting.h"
#include "common/strings/parsing.h"
#include "common/strings/utf8.h"
#include "common/unique_numbers.h"
#include "common/xml/ebml_chapters_converter.h"

namespace mtx::chapters {

namespace {
debugging_option_c s_debug{"chapters|chapter_parser"};
}

/** The default language for all chapter entries that don't have their own. */
mtx::bcp47::language_c g_default_language;
/** The default country for all chapter entries that don't have their own. */
std::string g_default_country;

translatable_string_c g_chapter_generation_name_template{YT("Chapter <NUM:2>")};

constexpr auto SIMCHAP_RE_TIMESTAMP_LINE = "^\\s*CHAPTER\\d+\\s*=\\s*(\\d+)\\s*:\\s*(\\d+)\\s*:\\s*(\\d+)\\s*[\\.,]\\s*(\\d{1,9})";
constexpr auto SIMCHAP_RE_TIMESTAMP      = "^\\s*CHAPTER\\d+\\s*=(.*)";
constexpr auto SIMCHAP_RE_NAME_LINE      = "^\\s*CHAPTER\\d+NAME\\s*=(.*)";

void
unify_legacy_and_bcp47_languages_and_countries(libebml::EbmlElement &elt) {
  auto master = dynamic_cast<libebml::EbmlMaster *>(&elt);
  if (!master)
    return;

  auto display = dynamic_cast<libmatroska::KaxChapterDisplay *>(&elt);
  if (!display) {
    for (auto const child : *master)
      unify_legacy_and_bcp47_languages_and_countries(*child);
    return;
  }

  std::vector<std::string> legacy_languages, legacy_countries;
  std::vector<mtx::bcp47::language_c> bcp47_languages;
  auto child_idx = 0u;

  while (child_idx < display->ListSize()) {
    auto remove_child = true;
    auto *child       = (*display)[child_idx];

    if (dynamic_cast<libmatroska::KaxChapterLanguage *>(child)) {
      auto legacy_language = static_cast<libmatroska::KaxChapterLanguage &>(*child).GetValue();
      if (!legacy_language.empty() && !mtx::includes(legacy_languages, legacy_language))
        legacy_languages.emplace_back(legacy_language);

    } else if (dynamic_cast<libmatroska::KaxChapterCountry *>(child)) {
      auto legacy_country = static_cast<libmatroska::KaxChapterCountry &>(*child).GetValue();
      if (!legacy_country.empty() && !mtx::includes(legacy_countries, legacy_country))
        legacy_countries.emplace_back(legacy_country);

    } else if (dynamic_cast<libmatroska::KaxChapLanguageIETF *>(child)) {
      auto bcp47_language = mtx::bcp47::language_c::parse(static_cast<libmatroska::KaxChapLanguageIETF &>(*child).GetValue());
      if (bcp47_language.is_valid() && !mtx::includes(bcp47_languages, bcp47_language))
        bcp47_languages.emplace_back(bcp47_language);

    } else
      remove_child = false;

    if (remove_child) {
      display->Remove(child_idx);
      delete child;

    } else
      ++child_idx;
  }

  if (legacy_languages.empty() && bcp47_languages.empty())
    legacy_languages.emplace_back("eng"s);

  if (bcp47_languages.empty()) {
    auto add_maybe = [&bcp47_languages](std::string const &new_bcp47_language_str) {
      auto new_bcp47_language = mtx::bcp47::language_c::parse(new_bcp47_language_str);
      if (new_bcp47_language.is_valid() && !mtx::includes(bcp47_languages, new_bcp47_language))
        bcp47_languages.emplace_back(new_bcp47_language);
    };

    for (auto const &legacy_language : legacy_languages) {
      if (legacy_countries.empty())
        add_maybe(legacy_language);

      else
        for (auto const &legacy_country : legacy_countries) {
          auto language_and_region = fmt::format("{0}-{1}", legacy_language, mtx::string::to_lower_ascii(legacy_country) == "uk" ? "gb"s : legacy_country);
          add_maybe(language_and_region);
        }
    }
  }

  legacy_languages.clear();
  legacy_countries.clear();

  for (auto const &bcp47_language : bcp47_languages) {
    auto legacy_language = bcp47_language.get_closest_iso639_2_alpha_3_code();

    if (!mtx::includes(legacy_languages, legacy_language))
      legacy_languages.emplace_back(legacy_language);

    auto legacy_country = bcp47_language.get_top_level_domain_country_code();

    if (!legacy_country.empty() && !mtx::includes(legacy_countries, legacy_country))
      legacy_countries.emplace_back(legacy_country);
  }

  std::sort(legacy_languages.begin(), legacy_languages.end());
  std::sort(legacy_countries.begin(), legacy_countries.end());
  std::sort(bcp47_languages.begin(),  bcp47_languages.end());

  for (auto const &legacy_language : legacy_languages)
    add_empty_child<libmatroska::KaxChapterLanguage>(display).SetValue(legacy_language);

  for (auto const &legacy_country : legacy_countries)
    add_empty_child<libmatroska::KaxChapterCountry>(display).SetValue(legacy_country);

  if (mtx::bcp47::language_c::is_disabled())
    return;

  for (auto const &bcp47_language : bcp47_languages)
    add_empty_child<libmatroska::KaxChapLanguageIETF>(display).SetValue(bcp47_language.format());
}

/** \brief Throw a special chapter parser exception.

   \param error The error message.
*/
inline void
chapter_error(const std::string &error) {
  throw parser_x(fmt::format(FY("Simple chapter parser: {0}\n"), error));
}

/** \brief Reads the start of a file and checks for OGM style comments.

   The first lines are read. OGM style comments are recognized if the first
   non-empty line contains <tt>CHAPTER01=...</tt> and the first non-empty
   line afterwards contains <tt>CHAPTER01NAME=...</tt>.

   The parameters are checked for validity.

   \param in The file to read from.

   \return \c true if the file contains OGM style comments and \c false
     otherwise.
*/
bool
probe_simple(mm_text_io_c *in) {
  QRegularExpression timestamp_line_re{SIMCHAP_RE_TIMESTAMP_LINE};
  QRegularExpression name_line_re{     SIMCHAP_RE_NAME_LINE};

  std::string line;

  assert(in);

  in->setFilePointer(0);
  while (in->getline2(line)) {
    mtx::string::strip(line);
    if (line.empty())
      continue;

    if (!Q(line).contains(timestamp_line_re))
      return false;

    while (in->getline2(line)) {
      mtx::string::strip(line);
      if (line.empty())
        continue;

      return Q(line).contains(name_line_re);
    }

    return false;
  }

  return false;
}

//           1         2
// 012345678901234567890123
//
// CHAPTER01=00:00:00.000
// CHAPTER01NAME=Hallo Welt

/** \brief Parse simple OGM style comments

   The file \a in is read. The content is assumed to be OGM style comments.

   The parameters are checked for validity.

   \param in The text file to read from.
   \param min_ts An optional timestamp. If both \a min_ts and \a max_ts are
     given then only those chapters that lie in the timerange
     <tt>[min_ts..max_ts]</tt> are kept.
   \param max_ts An optional timestamp. If both \a min_ts and \a max_ts are
     given then only those chapters that lie in the timerange
     <tt>[min_ts..max_ts]</tt> are kept.
   \param offset An optional offset that is subtracted from all start and
     end timestamps after the timerange check has been made.
   \param language This language is added as the \c libmatroska::KaxChapterLanguage
     for all entries.
   \param charset The charset the chapters are supposed to be it. The entries
     will be converted to UTF-8 if necessary.
   \param exception_on_error If set to \c true then an exception is thrown
     if an error occurs. Otherwise \c nullptr will be returned.

   \return The chapters parsed from the file or \c nullptr if an error occurred.
*/
kax_cptr
parse_simple(mm_text_io_c *in,
             int64_t min_ts,
             int64_t max_ts,
             int64_t offset,
             mtx::bcp47::language_c const &language,
             std::string const &charset) {
  assert(in);

  in->setFilePointer(0);

  kax_cptr chaps{new libmatroska::KaxChapters};
  libmatroska::KaxChapterAtom *atom     = nullptr;
  libmatroska::KaxEditionEntry *edition = nullptr;
  int mode                              = 0;
  int num                               = 0;
  int64_t start                         = 0;

  auto has_bom  = in->get_byte_order_mark() != byte_order_mark_e::none;
  auto try_utf8 = !has_bom && charset.empty();
  auto cc_utf8  = charset_converter_c::init(charset);

  auto use_language = language.is_valid()           ? language
                    : g_default_language.is_valid() ? g_default_language
                    :                                 mtx::bcp47::language_c::parse("eng"s);

  QRegularExpression timestamp_line_re{SIMCHAP_RE_TIMESTAMP_LINE};
  QRegularExpression timestamp_re{     SIMCHAP_RE_TIMESTAMP};
  QRegularExpression name_line_re{     SIMCHAP_RE_NAME_LINE};
  QRegularExpressionMatch matches;

  std::string line;

  while (in->getline2(line)) {
    if (!has_bom) {
      auto ok = false;

      if (try_utf8) {
        if (mtx::utf8::is_valid(line))
          ok = true;
        else
          try_utf8 = false;
      }

      if (!ok)
        line = cc_utf8->utf8(line);
    }

    mtx::utf8::fix_invalid(line);

    mtx::string::strip(line);
    if (line.empty())
      continue;

    if (0 == mode) {
      matches = timestamp_line_re.match(Q(line));
      if (!matches.hasMatch())
        chapter_error(fmt::format(FY("'{0}' is not a CHAPTERxx=... line."), line));

      int64_t hour = 0, minute = 0, second = 0, nsecs = 0;
      mtx::string::parse_number(to_utf8(matches.captured(1)), hour);
      mtx::string::parse_number(to_utf8(matches.captured(2)), minute);
      mtx::string::parse_number(to_utf8(matches.captured(3)), second);
      mtx::string::parse_number(to_utf8(matches.captured(4)), nsecs);

      if (59 < minute)
        chapter_error(fmt::format(FY("Invalid minute: {0}"), minute));
      if (59 < second)
        chapter_error(fmt::format(FY("Invalid second: {0}"), second));

      for (int idx = matches.capturedLength(4); idx < 9; ++idx)
        nsecs *= 10;

      start = nsecs + (second + minute * 60 + hour * 60 * 60) * 1'000'000'000;
      mode  = 1;

      if (matches = timestamp_re.match(Q(line)); !matches.hasMatch())
        chapter_error(fmt::format(FY("'{0}' is not a CHAPTERxx=... line."), line));

    } else {
      if (matches = name_line_re.match(Q(line)); !matches.hasMatch())
        chapter_error(fmt::format(FY("'{0}' is not a CHAPTERxxNAME=... line."), line));

      auto name = to_utf8(matches.captured(1));
      if (name.empty())
        name = format_name_template(g_chapter_generation_name_template.get_translated(), num + 1, timestamp_c::ns(start));

      mode = 0;

      if ((start >= min_ts) && ((start <= max_ts) || (max_ts == -1))) {
        if (!edition)
          edition = &get_child<libmatroska::KaxEditionEntry>(*chaps);

        atom = &get_first_or_next_child<libmatroska::KaxChapterAtom>(*edition, atom);
        get_child<libmatroska::KaxChapterUID>(*atom).SetValue(create_unique_number(UNIQUE_CHAPTER_IDS));
        get_child<libmatroska::KaxChapterTimeStart>(*atom).SetValue(start - offset);

        auto &display = get_child<libmatroska::KaxChapterDisplay>(*atom);

        get_child<libmatroska::KaxChapterString>(display).SetValueUTF8(name);
        if (use_language.is_valid()) {
          get_child<libmatroska::KaxChapterLanguage>(display).SetValue(use_language.get_closest_iso639_2_alpha_3_code());
          if (!mtx::bcp47::language_c::is_disabled())
            get_child<libmatroska::KaxChapLanguageIETF>(display).SetValue(use_language.format());
          else
            delete_children<libmatroska::KaxChapLanguageIETF>(display);
        }

        if (!g_default_country.empty())
          get_child<libmatroska::KaxChapterCountry>(display).SetValue(g_default_country);

        ++num;
      }
    }
  }

  return 0 == num ? kax_cptr{} : chaps;
}

bool
probe_ffmpeg_meta(mm_text_io_c *in) {
  std::string line;

  in->setFilePointer(0);

  if (!in->getline2(line))
    return false;

  mtx::string::strip(line);

  return line == ";FFMETADATA1";
}

kax_cptr
parse_ffmpeg_meta(mm_text_io_c *in,
                  int64_t min_ts,
                  int64_t max_ts,
                  int64_t offset,
                  mtx::bcp47::language_c const &language,
                  std::string const &charset) {
  in->setFilePointer(0);

  std::string line, title;
  std::optional<int64_t> start_scaled, end_scaled;
  mtx_mp_rational_t time_base;
  kax_cptr chapters;
  libmatroska::KaxEditionEntry *edition{};
  charset_converter_cptr cc_utf8;
  bool in_chapter = false;
  bool do_convert = in->get_byte_order_mark() == byte_order_mark_e::none;

  if (do_convert)
    cc_utf8 = charset_converter_c::init(charset);

  auto use_language = language.is_valid()           ? language
                    : g_default_language.is_valid() ? g_default_language
                    :                                 mtx::bcp47::language_c::parse("eng"s);

  QRegularExpression
    start_line_re{     Q("^start *=([0-9]+)"),             QRegularExpression::CaseInsensitiveOption},
    end_line_re{       Q("^end *=([0-9]+)"),               QRegularExpression::CaseInsensitiveOption},
    title_line_re{     Q("^title *=(.*)"),                 QRegularExpression::CaseInsensitiveOption},
    time_base_line_re{ Q("^timebase *=([0-9]+)/([0-9]+)"), QRegularExpression::CaseInsensitiveOption};
  QRegularExpressionMatch matches;

  auto reset_values = [&]() {
    start_scaled.reset();
    end_scaled.reset();
    title.clear();

    time_base = mtx::rational(1, 1);
  };

  auto add_chapter_atom = [&]() {
    if (!start_scaled || !in_chapter) {
      reset_values();
      return;
    }

    auto start = mtx::to_int_rounded(*start_scaled * time_base);

    if ((start < min_ts) || ((max_ts != -1) && (start > max_ts))) {
      reset_values();
      return;
    }

    if (!chapters) {
      chapters   = std::make_shared<libmatroska::KaxChapters>();
      edition = &get_child<libmatroska::KaxEditionEntry>(*chapters);
    }

    auto &atom = add_empty_child<libmatroska::KaxChapterAtom>(*edition);
    get_child<libmatroska::KaxChapterUID>(atom).SetValue(create_unique_number(UNIQUE_CHAPTER_IDS));
    get_child<libmatroska::KaxChapterTimeStart>(atom).SetValue(start - offset);

    if (end_scaled) {
      auto end = mtx::to_int_rounded(*end_scaled * time_base);
      get_child<libmatroska::KaxChapterTimeEnd>(atom).SetValue(end - offset);
    }

    auto &display = get_child<libmatroska::KaxChapterDisplay>(atom);
    get_child<libmatroska::KaxChapterString>(display).SetValueUTF8(title);

    if (use_language.is_valid()) {
      get_child<libmatroska::KaxChapterLanguage>(display).SetValue(use_language.get_closest_iso639_2_alpha_3_code());
      if (!mtx::bcp47::language_c::is_disabled())
        get_child<libmatroska::KaxChapLanguageIETF>(display).SetValue(use_language.format());
      else
        delete_children<libmatroska::KaxChapLanguageIETF>(display);
    }

    if (!g_default_country.empty())
      get_child<libmatroska::KaxChapterCountry>(display).SetValue(g_default_country);

    reset_values();
  };

  while (in->getline2(line)) {
    if (do_convert)
      line = cc_utf8->utf8(line);

    mtx::string::strip(line);
    if (line.empty() || (line[0] == ';') || (line[0] == '#'))
      continue;

    if (mtx::string::to_lower_ascii(line) == "[chapter]") {
      add_chapter_atom();
      in_chapter = true;

    } else if (line[0] == '[') {
      add_chapter_atom();
      in_chapter = false;

    } else if (!in_chapter)
      continue;

    else if ((matches = title_line_re.match(Q(line))).hasMatch())
      title = to_utf8(matches.captured(1));

    else if ((matches = start_line_re.match(Q(line))).hasMatch())
      start_scaled = matches.captured(1).toLongLong();

    else if ((matches = end_line_re.match(Q(line))).hasMatch())
      end_scaled = matches.captured(1).toLongLong();

    else if ((matches = time_base_line_re.match(Q(line))).hasMatch()) {
      auto numerator   = matches.captured(1).toLongLong();
      auto deniminator = matches.captured(2).toLongLong();

      if ((numerator != 0) && (deniminator != 0))
        time_base = mtx::rational(numerator, deniminator) * 1'000'000'000;
    }
  }

  add_chapter_atom();

  return chapters;
}

bool
probe_potplayer_bookmarks(mm_text_io_c *in) {
  std::string line;

  in->setFilePointer(0);

  if (!in->getline2(line))
    return false;

  mtx::string::strip(line);

  return line == "[Bookmark]";
}

kax_cptr
parse_potplayer_bookmarks(mm_text_io_c *in,
                          int64_t min_ts,
                          int64_t max_ts,
                          int64_t offset,
                          mtx::bcp47::language_c const &language,
                          std::string const &charset) {
  in->setFilePointer(0);

  std::string line;
  kax_cptr chapters;
  libmatroska::KaxEditionEntry *edition{};
  charset_converter_cptr cc_utf8;
  bool do_convert = in->get_byte_order_mark() == byte_order_mark_e::none;

  if (do_convert)
    cc_utf8 = charset_converter_c::init(charset);

  auto use_language = language.is_valid()           ? language
                    : g_default_language.is_valid() ? g_default_language
                    :                                 mtx::bcp47::language_c::parse("eng"s);

  QRegularExpression content_re{Q(R"(^[0-9]+=([0-9]+)\*(.*?)(\*[0-9a-fA-F]+)?$)")};
  QRegularExpressionMatch matches;

  while (in->getline2(line)) {
    if (do_convert)
      line = cc_utf8->utf8(line);

    mtx::string::strip(line);

    if (!(matches = content_re.match(Q(line))).hasMatch())
      continue;

    uint64_t timestamp_ms;
    if (!mtx::string::parse_number(to_utf8(matches.captured(1)), timestamp_ms))
      continue;

    auto timestamp_ns = timestamp_c::ms(timestamp_ms).to_ns();

    if ((timestamp_ns < min_ts) || ((max_ts != -1) && (timestamp_ns > max_ts)))
      continue;

    auto title = to_utf8(matches.captured(2));
    if (title.empty())
      continue;

    if (!chapters) {
      chapters   = std::make_shared<libmatroska::KaxChapters>();
      edition = &get_child<libmatroska::KaxEditionEntry>(*chapters);
    }

    auto &atom = add_empty_child<libmatroska::KaxChapterAtom>(*edition);
    get_child<libmatroska::KaxChapterUID>(atom).SetValue(create_unique_number(UNIQUE_CHAPTER_IDS));
    get_child<libmatroska::KaxChapterTimeStart>(atom).SetValue(timestamp_ns - offset);

    auto &display = get_child<libmatroska::KaxChapterDisplay>(atom);
    get_child<libmatroska::KaxChapterString>(display).SetValueUTF8(title);

    if (use_language.is_valid()) {
      get_child<libmatroska::KaxChapterLanguage>(display).SetValue(use_language.get_closest_iso639_2_alpha_3_code());
      if (!mtx::bcp47::language_c::is_disabled())
        get_child<libmatroska::KaxChapLanguageIETF>(display).SetValue(use_language.format());
      else
        delete_children<libmatroska::KaxChapLanguageIETF>(display);
    }

    if (!g_default_country.empty())
      get_child<libmatroska::KaxChapterCountry>(display).SetValue(g_default_country);
  }

  return chapters;
}

/** \brief Probe a file for different chapter formats and parse the file.

   The file \a file_name is opened and checked for supported chapter formats.
   These include simple OGM style chapters, cue sheets and mkvtoolnix' own
   XML chapter format.

   Its parameters don't have to be checked for validity.

   \param file_name The name of the text file to read from.
   \param min_ts An optional timestamp. If both \a min_ts and \a max_ts are
     given then only those chapters that lie in the timerange
     <tt>[min_ts..max_ts]</tt> are kept.
   \param max_ts An optional timestamp. If both \a min_ts and \a max_ts are
     given then only those chapters that lie in the timerange
     <tt>[min_ts..max_ts]</tt> are kept.
   \param offset An optional offset that is subtracted from all start and
     end timestamps after the timerange check has been made.
   \param language This language is added as the \c libmatroska::KaxChapterLanguage
     for entries that don't specifiy it.
   \param charset The charset the chapters are supposed to be it. The entries
     will be converted to UTF-8 if necessary. This parameter is ignored for XML
     chapter files.
   \param exception_on_error If set to \c true then an exception is thrown
     if an error occurs. Otherwise \c nullptr will be returned.
   \param format If given, this parameter will be set to the recognized chapter
     format. May be \c nullptr if the caller is not interested in the result.
   \param tags When parsing a cue sheet tags will be created along with the
     chapter entries. These tags will be stored in this parameter.

   \return The chapters parsed from the file or \c nullptr if an error occurred.

   \see ::parse_chapters(mm_text_io_c *in,int64_t min_ts,int64_t max_ts, int64_t offset,const mtx::bcp47::language_c &language,const std::string &charset,bool exception_on_error,format_e *format,KaxTags **tags)
*/
kax_cptr
parse(const std::string &file_name,
      int64_t min_ts,
      int64_t max_ts,
      int64_t offset,
      const mtx::bcp47::language_c &language,
      const std::string &charset,
      bool exception_on_error,
      format_e *format,
      std::unique_ptr<libmatroska::KaxTags> *tags) {
  try {
#if defined(HAVE_DVDREAD)
    auto parsed_dvd_chapters = maybe_parse_dvd(file_name, language);
    if (parsed_dvd_chapters) {
      unify_legacy_and_bcp47_languages_and_countries(*parsed_dvd_chapters);

      if (format)
        *format = format_e::dvd;

      return parsed_dvd_chapters;
    }
#endif

    auto parsed_bluray_chapters = maybe_parse_bluray(file_name, language);
    if (parsed_bluray_chapters) {
      unify_legacy_and_bcp47_languages_and_countries(*parsed_bluray_chapters);

      if (format)
        *format = format_e::bluray;

      return parsed_bluray_chapters;
    }

    mm_text_io_c in(std::make_shared<mm_file_io_c>(file_name));
    auto parsed_chapters = parse(&in, min_ts, max_ts, offset, language, charset, exception_on_error, format, tags);

    if (parsed_chapters)
      unify_legacy_and_bcp47_languages_and_countries(*parsed_chapters);

    return parsed_chapters;

  } catch (parser_x &e) {
    if (exception_on_error)
      throw;
    mxerror(fmt::format(FY("Could not parse the chapters in '{0}': {1}\n"), file_name, e.error()));

  } catch (...) {
    if (exception_on_error)
      throw parser_x(fmt::format(FY("Could not open '{0}' for reading.\n"), file_name));
    else
      mxerror(fmt::format(FY("Could not open '{0}' for reading.\n"), file_name));
  }

  return {};
}

/** \brief Probe a file for different chapter formats and parse the file.

   The file \a in is checked for supported chapter formats. These include
   simple OGM style chapters, cue sheets and mkvtoolnix' own XML chapter
   format.

   The parameters are checked for validity.

   \param in The text file to read from.
   \param min_ts An optional timestamp. If both \a min_ts and \a max_ts are
     given then only those chapters that lie in the timerange
     <tt>[min_ts..max_ts]</tt> are kept.
   \param max_ts An optional timestamp. If both \a min_ts and \a max_ts are
     given then only those chapters that lie in the timerange
     <tt>[min_ts..max_ts]</tt> are kept.
   \param offset An optional offset that is subtracted from all start and
     end timestamps after the timerange check has been made.
   \param language This language is added as the \c libmatroska::KaxChapterLanguage
     for entries that don't specifiy it.
   \param charset The charset the chapters are supposed to be it. The entries
     will be converted to UTF-8 if necessary. This parameter is ignored for XML
     chapter files.
   \param exception_on_error If set to \c true then an exception is thrown
     if an error occurs. Otherwise \c nullptr will be returned.
   \param format If given, this parameter will be set to the recognized chapter
     format. May be \c nullptr if the caller is not interested in the result.
   \param tags When parsing a cue sheet tags will be created along with the
     chapter entries. These tags will be stored in this parameter.

   \return The chapters parsed from the file or \c nullptr if an error occurred.

   \see ::parse_chapters(const std::string &file_name,int64_t min_ts,int64_t max_ts, int64_t offset,const mtx::bcp47::language_c &language,const std::string &charset,bool exception_on_error,format_e *format,std::unique_ptr<libmatroska::KaxTags> *tags)
*/
kax_cptr
parse(mm_text_io_c *in,
      int64_t min_ts,
      int64_t max_ts,
      int64_t offset,
      const mtx::bcp47::language_c &language,
      const std::string &charset,
      bool exception_on_error,
      format_e *format,
      std::unique_ptr<libmatroska::KaxTags> *tags) {
  assert(in);

  std::string error;

  try {
    if (probe_simple(in)) {
      if (format)
        *format = format_e::ogg;
      return parse_simple(in, min_ts, max_ts, offset, language, charset);

    } else if (probe_cue(in)) {
      if (format)
        *format = format_e::cue;
      return parse_cue(in, min_ts, max_ts, offset, language, charset, tags);

    } else if (probe_ffmpeg_meta(in)) {
      if (format)
        *format = format_e::ffmpeg_meta;
      return parse_ffmpeg_meta(in, min_ts, max_ts, offset, language, charset);

    } else if (probe_potplayer_bookmarks(in)) {
      if (format)
        *format = format_e::potplayer_bookmarks;
      return parse_potplayer_bookmarks(in, min_ts, max_ts, offset, language, charset);

    } else if (format)
      *format = format_e::xml;

    if (mtx::xml::ebml_chapters_converter_c::probe_file(in->get_file_name())) {
      auto chapters = mtx::xml::ebml_chapters_converter_c::parse_file(in->get_file_name(), true);
      return select_in_timeframe(chapters.get(), min_ts, max_ts, offset) ? chapters : nullptr;
    }

    error = fmt::format(FY("Unknown chapter file format in '{0}'. It does not contain a supported chapter format.\n"), in->get_file_name());
  } catch (mtx::chapters::parser_x &e) {
    error = e.error();
  } catch (mtx::mm_io::exception &ex) {
    error = fmt::format(FY("The XML chapter file '{0}' could not be read.\n"), in->get_file_name());
  } catch (mtx::xml::xml_parser_x &ex) {
    error = fmt::format(FY("The XML chapter file '{0}' contains an error at position {2}: {1}\n"), in->get_file_name(), ex.result().description(), ex.result().offset);
  } catch (mtx::xml::exception &ex) {
    error = fmt::format(FY("The XML chapter file '{0}' contains an error: {1}\n"), in->get_file_name(), ex.what());
  }

  if (!error.empty()) {
    if (exception_on_error)
      throw mtx::chapters::parser_x(error);
    mxerror(error);
  }

  return {};
}

/** \brief Get the start timestamp for a chapter atom.

   Its parameters don't have to be checked for validity.

   \param atom The atom for which the start timestamp should be returned.
   \param value_if_not_found The value to return if no start timestamp child
     element was found. Defaults to -1.

   \return The start timestamp or \c value_if_not_found if the atom doesn't
     contain such a child element.
*/
int64_t
get_start(libmatroska::KaxChapterAtom &atom,
          int64_t value_if_not_found) {
  auto start = find_child<libmatroska::KaxChapterTimeStart>(&atom);

  return !start ? value_if_not_found : static_cast<int64_t>(start->GetValue());
}

/** \brief Get the end timestamp for a chapter atom.

   Its parameters don't have to be checked for validity.

   \param atom The atom for which the end timestamp should be returned.
   \param value_if_not_found The value to return if no end timestamp child
     element was found. Defaults to -1.

   \return The start timestamp or \c value_if_not_found if the atom doesn't
     contain such a child element.
*/
int64_t
get_end(libmatroska::KaxChapterAtom &atom,
        int64_t value_if_not_found) {
  auto end = find_child<libmatroska::KaxChapterTimeEnd>(&atom);

  return !end ? value_if_not_found : static_cast<int64_t>(end->GetValue());
}

/** \brief Get the name for a chapter atom.

   Its parameters don't have to be checked for validity.

   \param atom The atom for which the name should be returned.

   \return The atom's name UTF-8 coded or \c "" if the atom doesn't contain
     such a child element.
*/
std::string
get_name(libmatroska::KaxChapterAtom &atom) {
  auto display = find_child<libmatroska::KaxChapterDisplay>(&atom);
  if (!display)
    return "";

  auto name = find_child<libmatroska::KaxChapterString>(display);
  if (!name)
    return "";

  return name->GetValueUTF8();
}

/** \brief Get the unique ID for a chapter atom.

   Its parameters don't have to be checked for validity.

   \param atom The atom for which the unique ID should be returned.

   \return The ID or \c -1 if the atom doesn't contain such a
     child element.
*/
int64_t
get_uid(libmatroska::KaxChapterAtom &atom) {
  auto uid = find_child<libmatroska::KaxChapterUID>(&atom);

  return !uid ? -1 : static_cast<int64_t>(uid->GetValue());
}

void
remove_elements_unsupported_by_webm(libebml::EbmlMaster &master) {
  static std::unordered_map<uint32_t, bool> s_supported_elements, s_readd_with_defaults;

  if (s_supported_elements.empty()) {
    s_supported_elements[ EBML_ID(libmatroska::KaxChapters).GetValue()            ] = true;
    s_supported_elements[ EBML_ID(libmatroska::KaxEditionEntry).GetValue()        ] = true;
    s_supported_elements[ EBML_ID(libmatroska::KaxChapterAtom).GetValue()         ] = true;
    s_supported_elements[ EBML_ID(libmatroska::KaxChapterUID).GetValue()          ] = true;
    s_supported_elements[ EBML_ID(libmatroska::KaxChapterStringUID).GetValue()    ] = true;
    s_supported_elements[ EBML_ID(libmatroska::KaxChapterTimeStart).GetValue()    ] = true;
    s_supported_elements[ EBML_ID(libmatroska::KaxChapterTimeEnd).GetValue()      ] = true;
    s_supported_elements[ EBML_ID(libmatroska::KaxChapterDisplay).GetValue()      ] = true;
    s_supported_elements[ EBML_ID(libmatroska::KaxChapterString).GetValue()       ] = true;
    s_supported_elements[ EBML_ID(libmatroska::KaxChapterLanguage).GetValue()     ] = true;
    s_supported_elements[ EBML_ID(libmatroska::KaxChapterCountry).GetValue()      ] = true;

    s_readd_with_defaults[ EBML_ID(libmatroska::KaxEditionFlagDefault).GetValue() ] = true;
    s_readd_with_defaults[ EBML_ID(libmatroska::KaxEditionFlagHidden).GetValue()  ] = true;
    s_readd_with_defaults[ EBML_ID(libmatroska::KaxChapterFlagEnabled).GetValue() ] = true;
    s_readd_with_defaults[ EBML_ID(libmatroska::KaxChapterFlagHidden).GetValue()  ] = true;
  }

  auto idx = 0u;

  while (idx < master.ListSize()) {
    auto e = master[idx];

    if (e && s_supported_elements[ get_ebml_id(*e).GetValue() ]) {
      auto sub_master = dynamic_cast<libebml::EbmlMaster *>(e);
      if (sub_master)
        remove_elements_unsupported_by_webm(*sub_master);

      ++idx;

      continue;
    }

    if (e && s_readd_with_defaults[ get_ebml_id(*e).GetValue() ]) {
      auto new_with_defaults = &(e->CreateElement());
      delete e;
      master.GetElementList()[idx] = new_with_defaults;

      ++idx;

      continue;
    }

    delete e;
    master.Remove(idx);
  }
}

/** \brief Remove all chapter atoms that are outside of a time range

   All chapter atoms that lie completely outside the timestamp range
   given with <tt>[min_ts..max_ts]</tt> are deleted. This is the workhorse
   for ::select_chapters_in_timeframe

   Chapters which start before the window but end inside or after the window
   are kept as well, and their start timestamp is adjusted.

   Its parameters don't have to be checked for validity.

   \param min_ts The minimum timestamp to accept.
   \param max_ts The maximum timestamp to accept.
   \param offset This value is subtracted from both the start and end timestamp
     for each chapter after the decision whether or not to keep it has been
     made.
   \param m The master containing the elements to check.
*/
static void
remove_entries(int64_t min_ts,
               int64_t max_ts,
               int64_t offset,
               libebml::EbmlMaster &m) {
  if (0 == m.ListSize())
    return;

  struct chapter_entry_t {
    bool remove{}, spans{}, is_atom{};
    int64_t start{}, end{-1};
  };
  std::vector<chapter_entry_t> entries;
  entries.resize(m.ListSize());

  unsigned int last_atom_at = 0;
  bool last_atom_found      = false;

  // Determine whether or not an entry has to be removed. Also retrieve
  // the start and end timestamps.
  size_t i;
  for (i = 0; m.ListSize() > i; ++i) {
    auto atom = dynamic_cast<libmatroska::KaxChapterAtom *>(m[i]);
    if (!atom)
      continue;

    last_atom_at       = i;
    last_atom_found    = true;
    entries[i].is_atom = true;

    auto cts = static_cast<libmatroska::KaxChapterTimeStart *>(atom->FindFirstElt(EBML_INFO(libmatroska::KaxChapterTimeStart), false));

    if (cts)
      entries[i].start = cts->GetValue();

    auto cte = static_cast<libmatroska::KaxChapterTimeEnd *>(atom->FindFirstElt(EBML_INFO(libmatroska::KaxChapterTimeEnd), false));

    if (cte)
      entries[i].end = cte->GetValue();
  }

  // We can return if we don't have a single atom to work with.
  if (!last_atom_found)
    return;

  for (i = 0; m.ListSize() > i; ++i) {
    auto atom = dynamic_cast<libmatroska::KaxChapterAtom *>(m[i]);
    if (!atom)
      continue;

    // Calculate the end timestamps and determine whether or not an entry spans
    // several segments.
    if (-1 == entries[i].end) {
      if (i == last_atom_at)
        entries[i].end = 1LL << 62;

      else {
        int next_atom = i + 1;

        while (!entries[next_atom].is_atom)
          ++next_atom;

        entries[i].end = entries[next_atom].start;
      }
    }

    if (   (entries[i].start < min_ts)
        || ((max_ts >= 0) && (entries[i].start > max_ts)))
      entries[i].remove = true;

    if (entries[i].remove && (entries[i].start < min_ts) && (entries[i].end > min_ts))
      entries[i].spans = true;

    mxdebug_if(s_debug, fmt::format("remove_chapters: entries[{0}]: remove {1} spans {2} start {3} end {4}\n", i, entries[i].remove, entries[i].spans, entries[i].start, entries[i].end));

    // Spanning entries must be kept, and their start timestamp must be
    // adjusted. Entries that are to be deleted will be deleted later and
    // have to be skipped for now.
    if (entries[i].remove && !entries[i].spans)
      continue;

    auto cts = static_cast<libmatroska::KaxChapterTimeStart *>(atom->FindFirstElt(EBML_INFO(libmatroska::KaxChapterTimeStart), false));
    auto cte = static_cast<libmatroska::KaxChapterTimeEnd *>(atom->FindFirstElt(EBML_INFO(libmatroska::KaxChapterTimeEnd), false));

    if (entries[i].spans)
      cts->SetValue(min_ts);

    cts->SetValue(cts->GetValue() - offset);

    if (cte) {
      int64_t end_ts = cte->GetValue();

      if ((max_ts >= 0) && (end_ts > max_ts))
        end_ts = max_ts;
      end_ts -= offset;

      cte->SetValue(end_ts);
    }

    auto m2 = dynamic_cast<libebml::EbmlMaster *>(m[i]);
    if (m2)
      remove_entries(min_ts, max_ts, offset, *m2);
  }

  // Now really delete those entries.
  i = m.ListSize();
  while (0 < i) {
    --i;
    if (entries[i].remove && !entries[i].spans) {
      delete m[i];
      m.Remove(i);
    }
  }
}

/** \brief Merge all chapter atoms sharing the same UID

   If two or more chapters with the same UID are encountered on the same
   level then those are merged into a single chapter. The start timestamp
   is the minimum start timestamp of all the chapters, and the end timestamp
   is the maximum end timestamp of all the chapters.

   The parameters do not have to be checked for validity.

   \param master The master containing the elements to check.
*/
void
merge_entries(libebml::EbmlMaster &master) {
  size_t master_idx;

  // Iterate over all children of the atomaster.
  for (master_idx = 0; master.ListSize() > master_idx; ++master_idx) {
    // Not every child is a chapter atomaster. Skip those.
    auto atom = dynamic_cast<libmatroska::KaxChapterAtom *>(master[master_idx]);
    if (!atom)
      continue;

    int64_t uid = get_uid(*atom);
    if (-1 == uid)
      continue;

    // First get the start and end time, if present.
    int64_t start_ts = get_start(*atom, 0);
    int64_t end_ts   = get_end(*atom);

    mxdebug_if(s_debug, fmt::format("chapters: merge_entries: looking for {0} with {1}, {2}\n", uid, start_ts, end_ts));

    // Now iterate over all remaining atoms and find those with the same
    // UID.
    size_t merge_idx = master_idx + 1;
    while (true) {
      libmatroska::KaxChapterAtom *merge_this = nullptr;
      for (; master.ListSize() > merge_idx; ++merge_idx) {
        auto cmp_atom = dynamic_cast<libmatroska::KaxChapterAtom *>(master[merge_idx]);
        if (!cmp_atom)
          continue;

        if (get_uid(*cmp_atom) == uid) {
          merge_this = cmp_atom;
          break;
        }
      }

      // If we haven't found an atom with the same UID then we're done here.
      if (!merge_this)
        break;

      // Do the merger! First get the start and end timestamps if present.
      int64_t merge_start_ts = get_start(*merge_this, 0);
      int64_t merge_end_ts   = get_end(*merge_this);

      // Then compare them to the ones we have for the soon-to-be merged
      // chapter and assign accordingly.
      if (merge_start_ts < start_ts)
        start_ts = merge_start_ts;

      if ((-1 == end_ts) || (merge_end_ts > end_ts))
        end_ts = merge_end_ts;

      // Move all chapter atoms from the merged entry into the target
      // entry so that they will be merged recursively as well.
      auto merge_child_idx = 0u;
      auto num_children    = merge_this->ListSize();

      while (merge_child_idx < num_children) {
        if (is_type<libmatroska::KaxChapterAtom>((*merge_this)[merge_child_idx])) {
          atom->PushElement(*(*merge_this)[merge_child_idx]);
          merge_this->Remove(merge_child_idx);
          --num_children;

        } else
          ++merge_child_idx;
      }

      mxdebug_if(s_debug, fmt::format("chapters: merge_entries:   found one at {0} with {1}, {2}; merged to {3}, {4}\n", merge_idx, merge_start_ts, merge_end_ts, start_ts, end_ts));

      // Finally remove the entry itself.
      delete master[merge_idx];
      master.Remove(merge_idx);
    }

    // Assign the start and end timestamp to the chapter. Only assign an
    // end timestamp if one was present in at least one of the merged
    // chapter atoms.
    get_child<libmatroska::KaxChapterTimeStart>(*atom).SetValue(start_ts);
    if (-1 != end_ts)
      get_child<libmatroska::KaxChapterTimeEnd>(*atom).SetValue(end_ts);
  }

  // Recusively merge atoms.
  for (master_idx = 0; master.ListSize() > master_idx; ++master_idx) {
    auto merge_master = dynamic_cast<libebml::EbmlMaster *>(master[master_idx]);
    if (merge_master)
      merge_entries(*merge_master);
  }
}

/** \brief Remove all chapter atoms that are outside of a time range

   All chapter atoms that lie completely outside the timestamp range
   given with <tt>[min_ts..max_ts]</tt> are deleted.

   Chapters which start before the window but end inside or after the window
   are kept as well, and their start timestamp is adjusted.

   If two or more chapters with the same UID are encountered on the same
   level then those are merged into a single chapter. The start timestamp
   is the minimum start timestamp of all the chapters, and the end timestamp
   is the maximum end timestamp of all the chapters.

   The parameters are checked for validity.

   \param chapters The chapters to check.
   \param min_ts The minimum timestamp to accept.
   \param max_ts The maximum timestamp to accept.
   \param offset This value is subtracted from both the start and end timestamp
     for each chapter after the decision whether or not to keep it has been
     made.

   \return \c false if all chapters were discarded, \c true otherwise
*/
bool
select_in_timeframe(libmatroska::KaxChapters *chapters,
                    int64_t min_ts,
                    int64_t max_ts,
                    int64_t offset) {
  // Check the parameters.
  if (!chapters)
    return false;

  // Remove the atoms that are outside of the requested range.
  size_t master_idx;
  for (master_idx = 0; chapters->ListSize() > master_idx; master_idx++) {
    auto work_master = dynamic_cast<libmatroska::KaxEditionEntry *>((*chapters)[master_idx]);
    if (work_master)
      remove_entries(min_ts, max_ts, offset, *work_master);
  }

  // Count the number of atoms in each edition. Delete editions without
  // any atom in them.
  master_idx = 0;
  while (chapters->ListSize() > master_idx) {
    auto eentry = dynamic_cast<libmatroska::KaxEditionEntry *>((*chapters)[master_idx]);
    if (!eentry) {
      master_idx++;
      continue;
    }

    size_t num_atoms = 0, eentry_idx;
    for (eentry_idx = 0; eentry->ListSize() > eentry_idx; eentry_idx++)
      if (dynamic_cast<libmatroska::KaxChapterAtom *>((*eentry)[eentry_idx]))
        num_atoms++;

    if (0 == num_atoms) {
      chapters->Remove(master_idx);
      delete eentry;

    } else
      master_idx++;
  }

  return chapters->ListSize() > 0;
}

/** \brief Find an edition with a specific UID.

   Its parameters don't have to be checked for validity.

   \param chapters The chapters in which to look for the edition.
   \param uid The requested unique edition ID. The special value \c 0
     results in the first edition being returned.

   \return A pointer to the edition or \c nullptr if none has been found.
*/
libmatroska::KaxEditionEntry *
find_edition_with_uid(libmatroska::KaxChapters &chapters,
                      uint64_t uid) {
  if (0 == uid)
    return find_child<libmatroska::KaxEditionEntry>(&chapters);

  size_t eentry_idx;
  for (eentry_idx = 0; chapters.ListSize() > eentry_idx; eentry_idx++) {
    auto eentry = dynamic_cast<libmatroska::KaxEditionEntry *>(chapters[eentry_idx]);
    if (!eentry)
      continue;

    auto euid = find_child<libmatroska::KaxEditionUID>(eentry);
    if (euid && (euid->GetValue() == uid))
      return eentry;
  }

  return nullptr;
}

/** \brief Find a chapter atom with a specific UID.

   Its parameters don't have to be checked for validity.

   \param chapters The chapters in which to look for the atom.
   \param uid The requested unique atom ID. The special value \c 0 results in
     the first atom in the first edition being returned.

   \return A pointer to the atom or \c nullptr if none has been found.
*/
libmatroska::KaxChapterAtom *
find_chapter_with_uid(libmatroska::KaxChapters &chapters,
                      uint64_t uid) {
  if (0 == uid) {
    auto eentry = find_child<libmatroska::KaxEditionEntry>(&chapters);
    if (!eentry)
      return nullptr;
    return find_child<libmatroska::KaxChapterAtom>(eentry);
  }

  size_t eentry_idx;
  for (eentry_idx = 0; chapters.ListSize() > eentry_idx; eentry_idx++) {
    auto eentry = dynamic_cast<libmatroska::KaxEditionEntry *>(chapters[eentry_idx]);
    if (!eentry)
      continue;

    size_t atom_idx;
    for (atom_idx = 0; eentry->ListSize() > atom_idx; atom_idx++) {
      auto atom = dynamic_cast<libmatroska::KaxChapterAtom *>((*eentry)[atom_idx]);
      if (!atom)
        continue;

      auto cuid = find_child<libmatroska::KaxChapterUID>(atom);
      if (cuid && (cuid->GetValue() == uid))
        return atom;
    }
  }

  return nullptr;
}

/** \brief Move all chapter atoms to another container keeping editions intact

   This function moves all chapter atoms from \a src to \a dst.
   If there's already an edition in \a dst with the same UID as the current
   one in \a src, then all atoms will be put into that edition. Otherwise
   the complete edition will simply be moved over.

   After processing \a src will be empty.

   Its parameters don't have to be checked for validity.

   \param dst The container the atoms and editions will be put into.
   \param src The container the atoms and editions will be taken from.
*/
void
move_by_edition(libmatroska::KaxChapters &dst,
                libmatroska::KaxChapters &src) {
  size_t src_idx;
  for (src_idx = 0; src.ListSize() > src_idx; src_idx++) {
    auto m = dynamic_cast<libebml::EbmlMaster *>(src[src_idx]);
    if (!m)
      continue;

    // Find an edition to which these atoms will be added.
    libmatroska::KaxEditionEntry *ee_dst = nullptr;
    auto euid_src                        = find_child<libmatroska::KaxEditionUID>(m);
    if (euid_src)
      ee_dst = find_edition_with_uid(dst, euid_src->GetValue());

    // No edition with the same UID found as the one we want to handle?
    // Then simply move the complete edition over.
    if (!ee_dst)
      dst.PushElement(*m);
    else {
      // Move all atoms from the old edition to the new one.
      size_t master_idx;
      for (master_idx = 0; m->ListSize() > master_idx; master_idx++)
        if (is_type<libmatroska::KaxChapterAtom>((*m)[master_idx]))
          ee_dst->PushElement(*(*m)[master_idx]);
        else
          delete (*m)[master_idx];

      m->RemoveAll();
      delete m;
    }
  }

  src.RemoveAll();
}

/** \brief Adjust all start and end timestamps by an offset

   All start and end timestamps are adjusted by an offset. This is done
   recursively.

   Its parameters don't have to be checked for validity.

   \param master A master containint the elements to adjust. This can be
     a libmatroska::KaxChapters, libmatroska::KaxEditionEntry or libmatroska::KaxChapterAtom object.
   \param offset The offset to add to each timestamp. Can be negative. If
     the resulting timestamp would be smaller than zero then it will be set
     to zero.
*/
void
adjust_timestamps(libebml::EbmlMaster &master,
                  int64_t offset,
                  mtx_mp_rational_t const &factor) {
  size_t master_idx;
  for (master_idx = 0; master.ListSize() > master_idx; master_idx++) {
    if (!is_type<libmatroska::KaxChapterAtom>(master[master_idx]))
      continue;

    auto atom  = static_cast<libmatroska::KaxChapterAtom *>(master[master_idx]);
    auto start = find_child<libmatroska::KaxChapterTimeStart>(atom);
    auto end   = find_child<libmatroska::KaxChapterTimeEnd>(atom);

    if (start)
      start->SetValue(std::max<int64_t>(mtx::to_int(factor * mtx_mp_rational_t{start->GetValue()}) + offset, 0));

    if (end)
      end->SetValue(std::max<int64_t>(mtx::to_int(factor * mtx_mp_rational_t{end->GetValue()}) + offset, 0));
  }

  for (master_idx = 0; master.ListSize() > master_idx; master_idx++) {
    auto work_master = dynamic_cast<libebml::EbmlMaster *>(master[master_idx]);
    if (work_master)
      adjust_timestamps(*work_master, offset, factor);
  }
}

static int
count_atoms_recursively(libebml::EbmlMaster &master,
                        int count) {
  size_t master_idx;

  for (master_idx = 0; master.ListSize() > master_idx; ++master_idx)
    if (is_type<libmatroska::KaxChapterAtom>(master[master_idx]))
      ++count;

    else if (dynamic_cast<libebml::EbmlMaster *>(master[master_idx]))
      count = count_atoms_recursively(*static_cast<libebml::EbmlMaster *>(master[master_idx]), count);

  return count;
}

int
count_atoms(libebml::EbmlMaster &master) {
  return count_atoms_recursively(master, 0);
}

/** \brief Change the chapter edition UIDs to a single value

   This function changes the UIDs of all editions for which the
   function is called to a single value. This is intended for chapters
   read from source files which do not provide their own edition UIDs
   (e.g. MP4 or OGM files) so that their chapters can be appended and
   don't end up in separate editions.

   \c chapters may be nullptr in which case nothing is done.

   \param dst chapters The chapter structure for which all edition
      UIDs will be changed.
*/
void
align_uids(libmatroska::KaxChapters *chapters) {
  if (!chapters)
    return;

  static uint64_t s_shared_edition_uid = 0;

  if (0 == s_shared_edition_uid)
    s_shared_edition_uid = create_unique_number(UNIQUE_CHAPTER_IDS);

  size_t idx;
  for (idx = 0; chapters->ListSize() > idx; ++idx) {
    auto edition_entry = dynamic_cast<libmatroska::KaxEditionEntry *>((*chapters)[idx]);
    if (!edition_entry)
      continue;

    get_child<libmatroska::KaxEditionUID>(*edition_entry).SetValue(s_shared_edition_uid);
  }
}

void
align_uids(libmatroska::KaxChapters &reference,
           libmatroska::KaxChapters &modify) {
  size_t reference_idx = 0, modify_idx = 0;

  while (true) {
    libmatroska::KaxEditionEntry *ee_reference = nullptr;;
    while ((reference.ListSize() > reference_idx) && !(ee_reference = dynamic_cast<libmatroska::KaxEditionEntry *>(reference[reference_idx])))
      ++reference_idx;

    if (!ee_reference)
      return;

    libmatroska::KaxEditionEntry *ee_modify = nullptr;;
    while ((modify.ListSize() > modify_idx) && !(ee_modify = dynamic_cast<libmatroska::KaxEditionEntry *>(modify[modify_idx])))
      ++modify_idx;

    if (!ee_modify)
      return;

    get_child<libmatroska::KaxEditionUID>(*ee_modify).SetValue(get_child<libmatroska::KaxEditionUID>(*ee_reference).GetValue());
    ++reference_idx;
    ++modify_idx;
  }
}

static void
regenerate_uids_worker(libebml::EbmlMaster &master,
                       std::unordered_map<uint64_t, uint64_t> &new_chapter_uids) {
  for (int idx = 0, end = master.ListSize(); end > idx; ++idx) {
    auto element     = master[idx];
    auto edition_uid = dynamic_cast<libmatroska::KaxEditionUID *>(element);

    if (edition_uid) {
      edition_uid->SetValue(create_unique_number(UNIQUE_EDITION_IDS));
      continue;
    }

    auto chapter_uid = dynamic_cast<libmatroska::KaxChapterUID *>(element);

    if (chapter_uid) {
      new_chapter_uids[chapter_uid->GetValue()] = create_unique_number(UNIQUE_CHAPTER_IDS);
      chapter_uid->SetValue(new_chapter_uids[chapter_uid->GetValue()]);
      continue;
    }

    auto sub_master = dynamic_cast<libebml::EbmlMaster *>(master[idx]);
    if (sub_master)
      regenerate_uids_worker(*sub_master, new_chapter_uids);
  }
}

void
regenerate_uids(libebml::EbmlMaster &master,
                libebml::EbmlMaster *tags) {
  std::unordered_map<uint64_t, uint64_t> new_chapter_uids;

  regenerate_uids_worker(master, new_chapter_uids);

  if (tags)
    change_values<libmatroska::KaxTagChapterUID>(*tags, new_chapter_uids);
}

std::string
format_name_template(std::string const &name_template,
                     int chapter_number,
                     timestamp_c const &start_timestamp,
                     std::string const &appended_file_name,
                     std::string const &appended_title) {
  auto name                 = name_template;
  auto number_re            = QRegularExpression{"<NUM(?::(\\d+))?>"};
  auto timestamp_re         = QRegularExpression{"<START(?::([^>]+))?>"};
  auto file_name_re         = QRegularExpression{"<FILE_NAME>"};
  auto file_name_ext_re     = QRegularExpression{"<FILE_NAME_WITH_EXT>"};
  auto title_re             = QRegularExpression{"<TITLE>"};
  auto appended_file_name_p = mtx::fs::to_path(appended_file_name);

  name = mtx::string::replace(name, number_re, [=](auto const &match) {
    auto number_str    = fmt::format("{0}", chapter_number);
    auto wanted_length = 1u;

    if (match.capturedLength(1) && !mtx::string::parse_number(to_utf8(match.captured(1)), wanted_length))
      wanted_length = 1;

    if (number_str.length() < wanted_length)
      number_str = std::string(wanted_length - number_str.length(), '0') + number_str;

    return Q(number_str);
  });

  name = mtx::string::replace(name, timestamp_re, [=](auto const &match) {
    auto format = match.capturedLength(1) ? to_utf8(match.captured(1)) : "%H:%M:%S"s;
    return Q(mtx::string::format_timestamp(start_timestamp.to_ns(), format));
  });

  return to_utf8(Q(name)
                 .replace(file_name_re,     Q(appended_file_name_p.stem()))
                 .replace(file_name_ext_re, Q(appended_file_name_p.filename()))
                 .replace(title_re,         Q(appended_title)));
}

void
fix_country_codes(libebml::EbmlMaster &chapters) {
  for (auto const &child : chapters) {
    auto sub_master = dynamic_cast<libebml::EbmlMaster *>(child);
    if (sub_master) {
      fix_country_codes(*sub_master);
      continue;
    }

    auto ccountry = dynamic_cast<libmatroska::KaxChapterCountry *>(child);
    if (!ccountry)
      continue;

    auto country_opt = mtx::iso3166::look_up_cctld(ccountry->GetValue());
    if (country_opt)
      ccountry->SetValue(mtx::string::to_lower_ascii(country_opt->alpha_2_code));
  }
}

std::shared_ptr<libmatroska::KaxChapters>
create_editions_and_chapters(std::vector<std::vector<timestamp_c>> const &editions_timestamps,
                             mtx::bcp47::language_c const &language,
                             std::string const &name_template) {
  auto chapters          = std::make_shared<libmatroska::KaxChapters>();
  auto use_name_template = !name_template.empty()        ? name_template
                         :                                 g_chapter_generation_name_template.get_translated();
  auto use_language      = language.is_valid()           ? language
                         : g_default_language.is_valid() ? g_default_language
                         :                                 mtx::bcp47::language_c::parse("eng");

  for (auto const &timestamps : editions_timestamps) {
    auto edition        = new libmatroska::KaxEditionEntry;
    auto chapter_number = 0u;

    chapters->PushElement(*edition);

    get_child<libmatroska::KaxEditionUID>(edition).SetValue(create_unique_number(UNIQUE_EDITION_IDS));

    for (auto const &timestamp : timestamps) {
      ++chapter_number;

      auto name = format_name_template(use_name_template, chapter_number, timestamp);
      auto atom = mtx::construct::cons<libmatroska::KaxChapterAtom>(new libmatroska::KaxChapterUID,       create_unique_number(UNIQUE_CHAPTER_IDS),
                                                                    new libmatroska::KaxChapterTimeStart, timestamp.to_ns());

      if (!name.empty())
        atom->PushElement(*mtx::construct::cons<libmatroska::KaxChapterDisplay>(new libmatroska::KaxChapterString,    name,
                                                                                new libmatroska::KaxChapterLanguage,  use_language.get_closest_iso639_2_alpha_3_code(),
                                                                                new libmatroska::KaxChapLanguageIETF, use_language.format()));

      edition->PushElement(*atom);
    }
  }

  return chapters;
}

void
set_languages_in_display(libmatroska::KaxChapterDisplay &display,
                         std::vector<mtx::bcp47::language_c> const &parsed_languages) {
  delete_children<libmatroska::KaxChapLanguageIETF>(display);
  delete_children<libmatroska::KaxChapterLanguage>(display);
  delete_children<libmatroska::KaxChapterCountry>(display);

  for (auto const &parsed_language : parsed_languages)
    if (parsed_language.is_valid())
      add_empty_child<libmatroska::KaxChapLanguageIETF>(display).SetValue(parsed_language.format());

  unify_legacy_and_bcp47_languages_and_countries(display);
}

void
set_languages_in_display(libmatroska::KaxChapterDisplay &display,
                         mtx::bcp47::language_c const &parsed_language) {
  if (parsed_language.is_valid())
    set_languages_in_display(display, std::vector<mtx::bcp47::language_c>{ parsed_language });
}

void
set_languages_in_display(libmatroska::KaxChapterDisplay &display,
                         std::string const &language) {
  set_languages_in_display(display, std::vector<mtx::bcp47::language_c>{ mtx::bcp47::language_c::parse(language) });
}

mtx::bcp47::language_c
get_language_from_display(libmatroska::KaxChapterDisplay &display,
                          std::string const &default_if_missing) {
  auto language = find_child_value<libmatroska::KaxChapLanguageIETF>(display);
  if (language.empty())
    language = find_child_value<libmatroska::KaxChapterLanguage>(display);

  return mtx::bcp47::language_c::parse(!language.empty() ? language : default_if_missing);
}

}
