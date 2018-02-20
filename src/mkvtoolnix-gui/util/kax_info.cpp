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
  emit error_found(Q(error));
}

void
KaxInfo::ui_show_element_info(int level,
                              std::string const &text,
                              int64_t position,
                              int64_t size) {
  if (p_func()->m_use_gui)
    emit element_info_found(level, Q(text), position, size);

  else
    kax_info_c::ui_show_element_info(level, text, position, size);
}

void
KaxInfo::ui_show_element(EbmlElement &e) {
  auto p = p_func();

  if (p->m_use_gui)
    emit element_found(p->m_level, &e);

  else
    kax_info_c::ui_show_element(e);
}

void
KaxInfo::ui_show_progress(int percentage,
                          std::string const &text) {
  emit progress_changed(percentage, Q(text));
}

void
KaxInfo::run() {
  auto p = p_func();

  emit started();

  auto result = p->m_in ? process_file() : open_and_process_file(p->m_source_file_name);

  emit finished(result);
}

void
KaxInfo::abort() {
  ::mtx::kax_info_c::abort();
}

}}}
