/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   extracts tags from Matroska files into other files

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/tags/tags.h"
#include "common/xml/ebml_tags_converter.h"
#include "extract/mkvextract.h"

bool
extract_tags(kax_analyzer_c &analyzer,
             options_c::mode_options_c &options) {
  auto tags_element = analyzer.read_all(EBML_INFO(libmatroska::KaxTags));

  auto tags = dynamic_cast<libmatroska::KaxTags *>(tags_element.get());
  if (!tags)
    return true;

  if (options.m_no_track_tags)
    mtx::tags::remove_track_tags(tags);

  if (tags->ListSize())
    mtx::xml::ebml_tags_converter_c::write_xml(*tags, *open_output_file(options.m_output_file_name));

  return true;
}
