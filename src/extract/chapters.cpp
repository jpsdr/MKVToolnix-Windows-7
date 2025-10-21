/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   extracts chapters from Matroska files into other files
   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <cassert>
#include <iostream>

#include <ebml/EbmlHead.h>
#include <ebml/EbmlStream.h>
#include <ebml/EbmlVoid.h>

#include "avilib.h"
#include "common/chapters/chapters.h"
#include "common/ebml.h"
#include "common/mm_io.h"
#include "common/mm_io_x.h"
#include "common/kax_analyzer.h"
#include "common/xml/ebml_chapters_converter.h"
#include "extract/mkvextract.h"



bool
extract_chapters(kax_analyzer_c &analyzer,
                 options_c::mode_options_c &options) {
  auto element  = analyzer.read_all(EBML_INFO(libmatroska::KaxChapters));
  auto chapters = dynamic_cast<libmatroska::KaxChapters *>(element.get());

  if (!chapters)
    return true;

  mtx::chapters::fix_country_codes(*chapters);
  auto output = open_output_file(options.m_output_file_name);

  if (!options.m_simple_chapter_format)
    mtx::xml::ebml_chapters_converter_c::write_xml(*chapters, *output);

  else
    mtx::chapters::write_simple(*chapters, *output, options.m_simple_chapter_language);

  return true;
}
