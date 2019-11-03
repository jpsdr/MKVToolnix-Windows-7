/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#if defined(HAVE_CMARK)

namespace mtx::markdown {

std::string to_html(std::string const &markdown_text, int options = 0);

}

#endif  // defined(HAVE_CMARK)
