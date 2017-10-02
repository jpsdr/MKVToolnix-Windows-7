/*
   mkvmerge GUI -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Matroska file analyzer (Qt interface)

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#if defined(HAVE_QT)

#include <QProgressDialog>

#include "common/kax_analyzer.h"

class QtKaxAnalyzer : public kax_analyzer_c {
private:
  QWidget *m_parent;
  int64_t m_size{};
  std::unique_ptr<QProgressDialog> m_progressDialog;

public:
  QtKaxAnalyzer(QWidget *parent, QString const &fileName);
  virtual ~QtKaxAnalyzer();

  virtual void show_progress_start(int64_t size) override;
  virtual bool show_progress_running(int percentage) override;
  virtual void show_progress_done() override;

public:
  static void displayUpdateElementResult(QWidget *parent, update_element_result_e result, QString const &message);
};

#endif  // HAVE_QT
