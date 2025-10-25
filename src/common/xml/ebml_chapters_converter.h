/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

  EBML/XML converter specialization for chapters

  Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/chapters/chapters.h"
#include "common/xml/ebml_converter.h"

namespace mtx::xml {

class ebml_chapters_converter_c: public ebml_converter_c {
public:
  ebml_chapters_converter_c();
  virtual ~ebml_chapters_converter_c();

protected:
  virtual void fix_xml(document_cptr &doc) const;
  virtual void fix_ebml(libebml::EbmlMaster &root) const;
  virtual void fix_edition_entry(libmatroska::KaxEditionEntry &eentry) const;
  virtual void fix_atom(libmatroska::KaxChapterAtom &atom) const;
  virtual void fix_chapter_display(libmatroska::KaxChapterDisplay &display) const;
  virtual void fix_chapter_display_languages_and_countries(libmatroska::KaxChapterDisplay &display) const;
  virtual void fix_edition_display(libmatroska::KaxEditionDisplay &display) const;
  virtual void fix_edition_display_languages(libmatroska::KaxEditionDisplay &display) const;

  void setup_maps();

public:
  static void write_xml(libmatroska::KaxChapters &chapters, mm_io_c &out);
  static bool probe_file(std::string const &file_name);
  static mtx::chapters::kax_cptr parse_file(std::string const &file_name, bool throw_on_error);
};

}
