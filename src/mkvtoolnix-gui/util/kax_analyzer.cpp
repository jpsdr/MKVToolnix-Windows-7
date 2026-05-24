/*
   mkvmerge GUI -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Matroska file analyzer (Qt interface)

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <QMessageBox>

#include "common/qt.h"
#include "mkvtoolnix-gui/util/kax_analyzer.h"

namespace mtx::gui::Util {

KaxAnalyzer::KaxAnalyzer(QWidget *parent,
                         QString const &fileName)
  : kax_analyzer_c{to_utf8(fileName)}
  , m_parent{parent}
{
}

void
KaxAnalyzer::show_progress_start(int64_t size) {
  m_size           = size;
  m_progressDialog = std::make_unique<QProgressDialog>(QY("The file is being analyzed."), QY("Cancel"), 0, 100, m_parent);
  m_progressDialog->setWindowModality(Qt::WindowModal);
}

bool
KaxAnalyzer::show_progress_running(int percentage) {
  if (!m_progressDialog)
    return false;

  m_progressDialog->setValue(percentage);
  return !m_progressDialog->wasCanceled();
}

void
KaxAnalyzer::show_progress_done() {
  if (m_progressDialog)
    m_progressDialog->setValue(100);
  m_progressDialog.reset();
}

void
KaxAnalyzer::displayUpdateElementResult(QWidget *parent,
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
                           Q("%1 %2").arg(message).arg(QY("The Matroska file was modified, but the meta seek entry could not be updated. This means that players might have a hard time finding the element. "
                                                          "Please use your favorite player to check this file.")));
      return;

    case kax_analyzer_c::uer_error_opening_for_reading:
      QMessageBox::critical(parent, QY("Error reading Matroska file"),
                            Q("%1 %2 %3")
                            .arg(message)
                            .arg(QY("The file could not be opened for reading."))
                            .arg(QY("Possible reasons are: the file is not a Matroska file; the file is write-protected; the file is locked by another process; you do not have permission to access the file.")));
      return;

    case kax_analyzer_c::uer_error_opening_for_writing:
      QMessageBox::critical(parent, QY("Error writing Matroska file"),
                            Q("%1 %2 %3")
                            .arg(message)
                            .arg(QY("The file could not be opened for writing."))
                            .arg(QY("Possible reasons are: the file is not a Matroska file; the file is write-protected; the file is locked by another process; you do not have permission to access the file.")));
      return;

    case kax_analyzer_c::uer_error_fixing_last_element_unknown_size_failed:
      QMessageBox::critical(parent, QY("Error writing Matroska file"),
                            Q("%1 %2 %3 %4 %5 %6")
                            .arg(message)
                            .arg(QY("The Matroska file's last element is set to an unknown size."))
                            .arg(QY("Due to the particular structure of the file this situation cannot be fixed automatically."))
                            .arg(QY("The file can be fixed by multiplexing it with mkvmerge again."))
                            .arg(QY("The process will be aborted."))
                            .arg(QY("The file has not been modified.")));
      return;

    default:
      QMessageBox::critical(parent, QY("Internal program error"), Q("%1 %2").arg(message).arg(QY("An unknown error occurred. The file has been modified.")));
  }
}

}
