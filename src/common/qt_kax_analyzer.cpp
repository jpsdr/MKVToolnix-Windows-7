/*
   mkvmerge GUI -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Matroska file analyzer (Qt interface)

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#if defined(HAVE_QT)

#include "common/qt.h"
#include "common/qt_kax_analyzer.h"
#include "common/qt_kax_analyzer.h"

QtKaxAnalyzer::QtKaxAnalyzer(QWidget *parent,
                             QString const &fileName)
  : kax_analyzer_c{to_utf8(fileName)}
  , m_parent{parent}
{
}

QtKaxAnalyzer::~QtKaxAnalyzer() {
}

void
QtKaxAnalyzer::show_progress_start(int64_t size) {
  m_size           = size;
  m_progressDialog = std::make_unique<QProgressDialog>(QY("The file is being analyzed."), QY("Cancel"), 0, 100, m_parent);
  m_progressDialog->setWindowModality(Qt::WindowModal);
}

bool
QtKaxAnalyzer::show_progress_running(int percentage) {
  m_progressDialog->setValue(percentage);
  return !m_progressDialog->wasCanceled();
}

void
QtKaxAnalyzer::show_progress_done() {
  m_progressDialog->setValue(100);
  m_progressDialog.reset();
}

#endif  // HAVE_QT
