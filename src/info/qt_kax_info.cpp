/*
   mkvinfo -- utility for gathering information about Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   A Qt GUI for mkvinfo

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/qt.h"
#include "info/qt_kax_info.h"

namespace mtx {

qt_kax_info_c::~qt_kax_info_c() {
}

void
qt_kax_info_c::ui_show_error(std::string const &error) {
  if (m_use_gui)
    emit error_found(Q(error));
  else
    kax_info_c::ui_show_error(error);
}

void
qt_kax_info_c::ui_show_element(int level,
                               std::string const &text,
                               int64_t position,
                               int64_t size) {
  if (!m_use_gui)
    kax_info_c::ui_show_element(level, text, position, size);

  else
    emit element_found(level, Q(0 <= position ? create_element_text(text, position, size) : text));
}

void
qt_kax_info_c::ui_show_progress(int percentage,
                                     std::string const &text) {
  emit progress_changed(percentage, Q(text));
}

}
