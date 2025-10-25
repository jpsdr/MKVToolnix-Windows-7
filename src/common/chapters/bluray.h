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

#include "common/bluray/mpls.h"

namespace libmatroska {
class KaxChapters;
}

namespace mtx::chapters {

std::shared_ptr<libmatroska::KaxChapters> convert_mpls_chapters_kax_chapters(mtx::bluray::mpls::chapters_t const &chapters, mtx::bcp47::language_c const &main_language, std::string const &name_template = {});
std::shared_ptr<libmatroska::KaxChapters> maybe_parse_bluray(std::string const &file_name, mtx::bcp47::language_c const &language);

}
