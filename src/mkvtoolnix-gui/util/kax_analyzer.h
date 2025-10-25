/*
   mkvmerge GUI -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Matroska file analyzer (Qt interface)

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <QProgressDialog>

#include "common/kax_analyzer.h"

namespace mtx::gui::Util {

class KaxAnalyzer : public kax_analyzer_c {
private:
  QWidget *m_parent;
  int64_t m_size{};
  std::unique_ptr<QProgressDialog> m_progressDialog;

public:
  KaxAnalyzer(QWidget *parent, QString const &fileName);
  virtual ~KaxAnalyzer() = default;

  virtual void show_progress_start(int64_t size) override;
  virtual bool show_progress_running(int percentage) override;
  virtual void show_progress_done() override;

public:
  static void displayUpdateElementResult(QWidget *parent, update_element_result_e result, QString const &message);
};

}
