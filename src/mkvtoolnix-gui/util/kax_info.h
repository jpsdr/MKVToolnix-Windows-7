/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   retrieves and displays information about a Matroska file (Qt binding)

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include <QObject>

#include "common/kax_info.h"
#include "mkvtoolnix-gui/util/runnable.h"

namespace mtx { namespace gui { namespace Util {

class KaxInfo: public QObject, public ::mtx::kax_info_c {
  Q_OBJECT;

public:
  KaxInfo() = default;
  KaxInfo(QString const &file_name);
  virtual ~KaxInfo();

  virtual void ui_show_error(std::string const &error) override;
  virtual void ui_show_element_info(int level, std::string const &text, int64_t position, int64_t size) override;
  virtual void ui_show_element(EbmlElement &e) override;
  virtual void ui_show_progress(int percentage, std::string const &text) override;

public slots:
  virtual void run();
  virtual void abort();

signals:
  void elementInfoFound(int level, QString const &text, int64_t position, int64_t size);
  void elementFound(int level, EbmlElement *e);
  void errorFound(const QString &message);
  void progressChanged(int percentage, const QString &text);

  void runStarted();
  void runFinished(mtx::kax_info_c::result_e result);
};

}}}

Q_DECLARE_METATYPE(::mtx::kax_info_c::result_e);
