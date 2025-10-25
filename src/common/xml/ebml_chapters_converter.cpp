/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

  EBML/XML converter specialization for chapters

  Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <sstream>

#include "common/iso639.h"
#include "common/iso3166.h"
#include "common/mm_io_x.h"
#include "common/mm_file_io.h"
#include "common/mm_proxy_io.h"
#include "common/mm_text_io.h"
#include "common/strings/formatting.h"
#include "common/unique_numbers.h"
#include "common/xml/ebml_chapters_converter.h"

namespace mtx::xml {

ebml_chapters_converter_c::ebml_chapters_converter_c()
{
  setup_maps();
}

ebml_chapters_converter_c::~ebml_chapters_converter_c() {
}

void
ebml_chapters_converter_c::setup_maps() {
  m_formatter_map["ChapterTimeStart"]  = format_timestamp;
  m_formatter_map["ChapterTimeEnd"]    = format_timestamp;

  m_parser_map["ChapterTimeStart"]     = parse_timestamp;
  m_parser_map["ChapterTimeEnd"]       = parse_timestamp;

  m_limits["EditionUID"]               = limits_t{ true, false, 1, 0 };
  m_limits["EditionFlagHidden"]        = limits_t{ true, true,  0, 1 };
  m_limits["EditionFlagDefault"]       = limits_t{ true, true,  0, 1 };
  m_limits["EditionFlagOrdered"]       = limits_t{ true, true,  0, 1 };
  m_limits["ChapterFlagHidden"]        = limits_t{ true, true,  0, 1 };
  m_limits["ChapterFlagEnabled"]       = limits_t{ true, true,  0, 1 };
  m_limits["ChapterUID"]               = limits_t{ true, false, 1, 0 };
  m_limits["ChapterSegmentUID"]        = limits_t{ true, false, 1, 0 };
  m_limits["ChapterSegmentEditionUID"] = limits_t{ true, false, 1, 0 };
  m_limits["ChapterTrackNumber"]       = limits_t{ true, false, 1, 0 };
  m_limits["ChapterSkipType"]          = limits_t{ true, true,  0, 6 };

  reverse_debug_to_tag_name_map();

  if (debugging_c::requested("ebml_converter_semantics"))
    dump_semantics("Chapters");
}

void
ebml_chapters_converter_c::fix_xml(document_cptr &doc)
  const {
  auto result = doc->select_nodes("//ChapterAtom[not(ChapterTimeStart)]");
  for (auto &atom : result)
    atom.node().append_child("ChapterTimeStart").append_child(pugi::node_pcdata).set_value(mtx::string::format_timestamp(0).c_str());

  result = doc->select_nodes("//ChapterDisplay[not(ChapterString)]");
  for (auto &atom : result)
    atom.node().append_child("ChapterString");
}

void
ebml_chapters_converter_c::fix_ebml(libebml::EbmlMaster &chapters)
  const {
  for (auto element : chapters)
    if (dynamic_cast<libmatroska::KaxEditionEntry *>(element))
      fix_edition_entry(static_cast<libmatroska::KaxEditionEntry &>(*element));
}

void
ebml_chapters_converter_c::fix_edition_entry(libmatroska::KaxEditionEntry &eentry)
  const {
  bool atom_found = false;

  libmatroska::KaxEditionUID *euid = nullptr;
  for (auto element : eentry)
    if (dynamic_cast<libmatroska::KaxEditionUID *>(element)) {
      euid = static_cast<libmatroska::KaxEditionUID *>(element);
      if (!is_unique_number(euid->GetValue(), UNIQUE_EDITION_IDS)) {
        mxwarn(fmt::format(FY("Chapter parser: The EditionUID {0} is not unique and could not be reused. A new one will be created.\n"), euid->GetValue()));
        euid->SetValue(create_unique_number(UNIQUE_EDITION_IDS));
      }

    } else if (dynamic_cast<libmatroska::KaxEditionDisplay *>(element))
      fix_edition_display(static_cast<libmatroska::KaxEditionDisplay &>(*element));

    else if (dynamic_cast<libmatroska::KaxChapterAtom *>(element)) {
      atom_found = true;
      fix_atom(static_cast<libmatroska::KaxChapterAtom &>(*element));
    }

  if (!atom_found)
    throw conversion_x{Y("At least one <ChapterAtom> element is needed.")};

  if (!euid)
    eentry.PushElement((new libmatroska::KaxEditionUID)->SetValue(create_unique_number(UNIQUE_EDITION_IDS)));
}

void
ebml_chapters_converter_c::fix_atom(libmatroska::KaxChapterAtom &atom)
  const {
  for (auto element : atom)
    if (dynamic_cast<libmatroska::KaxChapterAtom *>(element))
      fix_atom(*static_cast<libmatroska::KaxChapterAtom *>(element));

  if (!find_child<libmatroska::KaxChapterTimeStart>(atom))
    throw conversion_x{Y("<ChapterAtom> is missing the <ChapterTimeStart> child.")};

  if (!find_child<libmatroska::KaxChapterUID>(atom))
    atom.PushElement((new libmatroska::KaxChapterUID)->SetValue(create_unique_number(UNIQUE_CHAPTER_IDS)));

  auto ctrack = find_child<libmatroska::KaxChapterTrack>(atom);
  if (ctrack && !find_child<libmatroska::KaxChapterTrackNumber>(ctrack))
    throw conversion_x{Y("<ChapterTrack> is missing the <ChapterTrackNumber> child.")};

  auto cdisplay = find_child<libmatroska::KaxChapterDisplay>(atom);
  if (cdisplay)
    fix_chapter_display(*cdisplay);
}

void
ebml_chapters_converter_c::fix_chapter_display_languages_and_countries(libmatroska::KaxChapterDisplay &display)
  const {
  for (auto const &child : display)
    if (auto kax_ietf_language = dynamic_cast<libmatroska::KaxChapLanguageIETF *>(child); kax_ietf_language) {
      auto parsed_language = mtx::bcp47::language_c::parse(kax_ietf_language->GetValue());

      if (!parsed_language.is_valid())
        throw conversion_x{fmt::format(FY("'{0}' is not a valid IETF BCP 47/RFC 5646 language tag. Additional information from the parser: {1}"), kax_ietf_language->GetValue(), parsed_language.get_error())};

    } else if (auto kax_legacy_language = dynamic_cast<libmatroska::KaxChapterLanguage *>(child); kax_legacy_language) {
      auto code         = kax_legacy_language->GetValue();
      auto language_opt = mtx::iso639::look_up(code);

      if (!language_opt || !language_opt->is_part_of_iso639_2)
        throw conversion_x{fmt::format(FY("'{0}' is not a valid ISO 639-2 language code."), code)};

    } else if (auto kax_country = dynamic_cast<libmatroska::KaxChapterCountry *>(child); kax_country) {
      auto country     = kax_country->GetValue();
      auto country_opt = mtx::iso3166::look_up_cctld(country);
      if (!country_opt)
        throw conversion_x{fmt::format(FY("'{0}' is not a valid ccTLD country code."), country)};

      auto cctld = mtx::string::to_lower_ascii(country_opt->alpha_2_code);

      if (country != cctld)
        kax_country->SetValue(cctld);
    }

  mtx::chapters::unify_legacy_and_bcp47_languages_and_countries(display);
}

void
ebml_chapters_converter_c::fix_chapter_display(libmatroska::KaxChapterDisplay &display)
  const {
  if (!find_child<libmatroska::KaxChapterString>(display))
    throw conversion_x{Y("<ChapterDisplay> is missing the <ChapterString> child.")};

  fix_chapter_display_languages_and_countries(display);
}

void
ebml_chapters_converter_c::fix_edition_display_languages(libmatroska::KaxEditionDisplay &display)
  const {
  for (auto const &child : display)
    if (auto kax_ietf_language = dynamic_cast<libmatroska::KaxEditionLanguageIETF *>(child); kax_ietf_language) {
      auto parsed_language = mtx::bcp47::language_c::parse(kax_ietf_language->GetValue());

      if (!parsed_language.is_valid())
        throw conversion_x{fmt::format(FY("'{0}' is not a valid IETF BCP 47/RFC 5646 language tag. Additional information from the parser: {1}"), kax_ietf_language->GetValue(), parsed_language.get_error())};
    }
}

void
ebml_chapters_converter_c::fix_edition_display(libmatroska::KaxEditionDisplay &display)
  const {
  if (!find_child<libmatroska::KaxEditionString>(display))
    throw conversion_x{Y("<EditionDisplay> is missing the <EditionString> child.")};

  fix_edition_display_languages(display);
}

void
ebml_chapters_converter_c::write_xml(libmatroska::KaxChapters &chapters,
                                     mm_io_c &out) {
  document_cptr doc(new pugi::xml_document);

  doc->append_child(pugi::node_comment).set_value(" <!DOCTYPE Chapters SYSTEM \"matroskachapters.dtd\"> ");

  ebml_chapters_converter_c converter;
  converter.to_xml(chapters, doc);

  out.write_bom("UTF-8");

  std::stringstream out_stream;
  doc->save(out_stream, "  ");
  out.puts(out_stream.str());
}

bool
ebml_chapters_converter_c::probe_file(std::string const &file_name) {
  try {
    mm_text_io_c in(std::make_shared<mm_file_io_c>(file_name, libebml::MODE_READ));
    std::string line;

    while (in.getline2(line)) {
      // I assume that if it looks like XML then it is an XML chapter file :)
      mtx::string::strip(line);
      if (balg::istarts_with(line, "<?xml"))
        return true;
      else if (!line.empty())
        return false;
    }

  } catch (...) {
  }

  return false;
}

mtx::chapters::kax_cptr
ebml_chapters_converter_c::parse_file(std::string const &file_name,
                                      bool throw_on_error) {
  auto parse = [&file_name]() -> auto {
    auto master = ebml_chapters_converter_c{}.to_ebml(file_name, "Chapters");
    sort_ebml_master(master.get());
    fix_mandatory_elements(static_cast<libmatroska::KaxChapters *>(master.get()));
    return std::dynamic_pointer_cast<libmatroska::KaxChapters>(master);
  };

  if (throw_on_error)
    return parse();

  try {
    return parse();

  } catch (mtx::mm_io::exception &ex) {
    mxerror(fmt::format(FY("The XML chapter file '{0}' could not be read.\n"), file_name));

  } catch (mtx::xml::xml_parser_x &ex) {
    mxerror(fmt::format(FY("The XML chapter file '{0}' contains an error at position {2}: {1}\n"), file_name, ex.result().description(), ex.result().offset));

  } catch (mtx::xml::exception &ex) {
    mxerror(fmt::format(FY("The XML chapter file '{0}' contains an error: {1}\n"), file_name, ex.what()));
  }

  return mtx::chapters::kax_cptr{};
}

}
