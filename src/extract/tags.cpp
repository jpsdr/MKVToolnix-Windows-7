/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   extracts tags from Matroska files into other files

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <cassert>
#include <iostream>

#include <ebml/EbmlHead.h>
#include <ebml/EbmlStream.h>
#include <ebml/EbmlVoid.h>

#include "common/ebml.h"
#include "common/kax_analyzer.h"
#include "common/mm_io.h"
#include "common/mm_io_x.h"
#include "common/xml/ebml_tags_converter.h"
#include "extract/mkvextract.h"

bool
extract_tags(kax_analyzer_c &analyzer,
             options_c::mode_options_c &options) {
  auto tags = analyzer.read_all(EBML_INFO(libmatroska::KaxTags));

  if (!dynamic_cast<libmatroska::KaxTags *>(tags.get()))
    return true;

  mtx::xml::ebml_tags_converter_c::write_xml(static_cast<libmatroska::KaxTags &>(*tags), *open_output_file(options.m_output_file_name));

  return true;
}
