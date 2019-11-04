/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit http://www.gnu.org/copyleft/gpl.html

  helper functions for chapters on Blu-rays

  Written by Moritz Bunkus <moritz@bunkus.org>.
*/
#include "common/common_pch.h"

#include "common/bluray/mpls.h"

namespace libmatroska {
class KaxChapters;
}

namespace mtx::chapters {

std::shared_ptr<libmatroska::KaxChapters> convert_mpls_chapters_kax_chapters(mtx::bluray::mpls::chapters_t const &chapters, std::string const &main_language, std::string const &name_template = {});

}
