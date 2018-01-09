/*
   mkvinfo -- utility for gathering information about Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include <matroska/KaxCluster.h>

#include "common/kax_element_names.h"
#include "common/qt.h"
#include "common/kax_info_p.h"
#include "mkvtoolnix-gui/util/kax_info.h"

namespace mtx { namespace gui { namespace Util {

KaxInfo::KaxInfo(QString const &file_name)
{
  set_source_file_name(to_utf8(file_name));
}

KaxInfo::~KaxInfo() {
}

void
KaxInfo::ui_show_error(std::string const &error) {
  if (!p_func()->m_use_gui)
    kax_info_c::ui_show_error(error);
  else
    emit error_found(Q(error));
}

void
KaxInfo::ui_show_element_info(int level,
                              std::string const &text,
                              int64_t position,
                              int64_t size) {
  auto p = p_func();

  if (!p->m_use_gui || !p->m_destination_file_name.empty())
    kax_info_c::ui_show_element_info(level, text, position, size);

  else {
    emit element_info_found(level, Q(text), position, size);
  }
}

void
KaxInfo::ui_show_element(EbmlElement &e) {
  auto p = p_func();

  if (!p->m_use_gui || !p->m_destination_file_name.empty())
    kax_info_c::ui_show_element(e);

  else {
    emit element_found(p->m_level, &e);
  }
}

void
KaxInfo::ui_show_progress(int percentage,
                          std::string const &text) {
  emit progress_changed(percentage, Q(text));
}

kax_info_c::result_e
KaxInfo::process_file() {
  emit started();

  auto result = kax_info_c::process_file();

  emit finished(result);

  return result;
}

void
KaxInfo::run() {
  auto p = p_func();

  if (p->m_in)
    process_file();
  else
    open_and_process_file(p->m_source_file_name);
}

void
KaxInfo::abort() {
  ::mtx::kax_info_c::abort();
}

}}}
