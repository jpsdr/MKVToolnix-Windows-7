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

#include <QMessageBox>

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

void
QtKaxAnalyzer::displayUpdateElementResult(QWidget *parent,
                                          update_element_result_e result,
                                          QString const &message) {
  switch (result) {
    case kax_analyzer_c::uer_success:
      return;

    case kax_analyzer_c::uer_error_segment_size_for_element:
      QMessageBox::critical(parent, QY("Error writing Matroska file"),
                            Q("%1 %2").arg(message).arg(QY("The element was written at the end of the file, but the segment size could not be updated. Therefore the element will not be visible. "
                                                           "The process will be aborted. The file has been changed!")));
      return;

    case kax_analyzer_c::uer_error_segment_size_for_meta_seek:
      QMessageBox::critical(parent, QY("Error writing Matroska file"),
                            Q("%1 %2").arg(message).arg(QY("The meta seek element was written at the end of the file, but the segment size could not be updated. Therefore the element will not be visible. "
                                                           "The process will be aborted. The file has been changed!")));
      return;

    case kax_analyzer_c::uer_error_meta_seek:
      QMessageBox::warning(parent, QY("File structure warning"),
                           Q("%1 %2").arg(message).arg(QY("The Matroska file was modified, but the meta seek entry could not be updated. This means that players might have a hard time finding the this element. "
                                                          "Please use your favorite player to check this file.")));
      return;

    default:
      QMessageBox::critical(parent, QY("Internal program error"), Q("%1 %2").arg(message).arg(QY("An unknown error occured. The file has been modified.")));
  }
}

#endif  // HAVE_QT
