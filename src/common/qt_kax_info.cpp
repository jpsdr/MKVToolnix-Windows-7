/*
   mkvinfo -- utility for gathering information about Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   A Qt GUI for mkvinfo

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#if defined(HAVE_QT)

# include "common/qt.h"
# include "common/qt_kax_info.h"

namespace mtx {

qt_kax_info_c::~qt_kax_info_c() {
}

void
qt_kax_info_c::ui_show_error(std::string const &error) {
  if (!m_use_gui)
    kax_info_c::ui_show_error(error);
  else
    emit error_found(Q(error));
}

void
qt_kax_info_c::ui_show_element(int level,
                               std::string const &text,
                               int64_t position,
                               int64_t size) {
  if (!m_use_gui || !m_destination_file_name.empty())
    kax_info_c::ui_show_element(level, text, position, size);

  else {
    emit element_found(level, Q(text), position, size);
  }
}

void
qt_kax_info_c::ui_show_progress(int percentage,
                                std::string const &text) {
  emit progress_changed(percentage, Q(text));
}

kax_info_c::result_e
qt_kax_info_c::process_file(std::string const &file_name) {
  emit started();

  auto result = kax_info_c::process_file(file_name);

  emit finished(result);

  return result;
}

void
qt_kax_info_c::set_source_file_name(std::string const &file_name) {
  m_source_file_name = file_name;
}

void
qt_kax_info_c::start_processing() {
  process_file(m_source_file_name);
}

}

#endif  // defined(HAVE_QT)
