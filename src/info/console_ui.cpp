/*
   mkvinfo -- utility for gathering information about Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   A console UI for mkvinfo

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "info/mkvinfo.h"

#if !defined(HAVE_QT)
int
ui_run(int /* argc */,
       char **/* argv */,
       options_c const &/* options */) {
  return 0;
}

bool
ui_graphical_available() {
  return false;
}

#endif  // !HAVE_QT
