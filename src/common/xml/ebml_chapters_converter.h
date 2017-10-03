/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit http://www.gnu.org/copyleft/gpl.html

  EBML/XML converter specialization for chapters

  Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/chapters/chapters.h"
#include "common/xml/ebml_converter.h"

using namespace libmatroska;

namespace mtx { namespace xml {

class ebml_chapters_converter_c: public ebml_converter_c {
public:
  ebml_chapters_converter_c();
  virtual ~ebml_chapters_converter_c();

protected:
  virtual void fix_xml(document_cptr &doc) const;
  virtual void fix_ebml(EbmlMaster &root) const;
  virtual void fix_edition_entry(KaxEditionEntry &eentry) const;
  virtual void fix_atom(KaxChapterAtom &atom) const;
  virtual void fix_display(KaxChapterDisplay &display) const;

  void setup_maps();

public:
  static void write_xml(KaxChapters &chapters, mm_io_c &out);
  static bool probe_file(std::string const &file_name);
  static mtx::chapters::kax_cptr parse_file(std::string const &file_name, bool throw_on_error);
};

}}
