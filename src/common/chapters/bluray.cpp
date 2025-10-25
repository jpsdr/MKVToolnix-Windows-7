/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

  helper functions for chapters on Blu-rays

  Written by Moritz Bunkus <mo@bunkus.online>.
*/
#include "common/common_pch.h"

#include "common/chapters/bluray.h"
#include "common/chapters/chapters.h"
#include "common/construct.h"
#include "common/ebml.h"
#include "common/hacks.h"
#include "common/mm_file_io.h"
#include "common/unique_numbers.h"

namespace mtx::chapters {

std::shared_ptr<libmatroska::KaxChapters>
convert_mpls_chapters_kax_chapters(mtx::bluray::mpls::chapters_t const &mpls_chapters,
                                   mtx::bcp47::language_c const &main_language_,
                                   std::string const &name_template_) {
  auto main_language  = !main_language_.has_valid_iso639_code() || (main_language_.get_iso639_alpha_3_code() == "und") ? mtx::bcp47::language_c::parse("eng"s) : main_language_;
  auto name_template  = name_template_.empty() ? mtx::chapters::g_chapter_generation_name_template.get_translated() : name_template_;
  auto chapter_number = 0;
  auto kax_chapters   = std::make_shared<libmatroska::KaxChapters>();
  auto &edition       = get_child<libmatroska::KaxEditionEntry>(*kax_chapters);

  get_child<libmatroska::KaxEditionUID>(edition).SetValue(create_unique_number(UNIQUE_EDITION_IDS));

  for (auto const &entry : mpls_chapters) {
    ++chapter_number;

    std::string name;
    for (auto const &[entry_language, entry_name] : entry.names)
      if (entry_language == main_language) {
        name = entry_name;
        break;
      }

    if (name.empty())
      name = mtx::chapters::format_name_template(name_template, chapter_number, entry.timestamp);

    auto atom = mtx::construct::cons<libmatroska::KaxChapterAtom>(new libmatroska::KaxChapterUID,       create_unique_number(UNIQUE_CHAPTER_IDS),
                                                                  new libmatroska::KaxChapterTimeStart, entry.timestamp.to_ns());

    if (!name.empty())
      atom->PushElement(*mtx::construct::cons<libmatroska::KaxChapterDisplay>(new libmatroska::KaxChapterString,   name,
                                                                              new libmatroska::KaxChapterLanguage, main_language.get_closest_iso639_2_alpha_3_code()));

    for (auto const &[entry_language, entry_name] : entry.names)
      if ((entry_language != main_language) && !entry_name.empty())
        atom->PushElement(*mtx::construct::cons<libmatroska::KaxChapterDisplay>(new libmatroska::KaxChapterString,   entry_name,
                                                                                new libmatroska::KaxChapterLanguage, entry_language.get_closest_iso639_2_alpha_3_code()));

    edition.PushElement(*atom);
  }

  mtx::chapters::align_uids(kax_chapters.get());
  mtx::chapters::unify_legacy_and_bcp47_languages_and_countries(*kax_chapters);

  return kax_chapters;
}

std::shared_ptr<libmatroska::KaxChapters>
maybe_parse_bluray(std::string const &file_name,
                   mtx::bcp47::language_c const &language) {

  mm_file_io_c in{file_name};
  auto parser = mtx::bluray::mpls::parser_c{};

  parser.enable_dropping_last_entry_if_at_end(!mtx::hacks::is_engaged(mtx::hacks::KEEP_LAST_CHAPTER_IN_MPLS));

  if (parser.parse(in))
    return mtx::chapters::convert_mpls_chapters_kax_chapters(parser.get_chapters(), language, g_chapter_generation_name_template.get_translated());

  return {};
}

}
