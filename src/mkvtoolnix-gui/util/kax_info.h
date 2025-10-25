/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   retrieves and displays information about a Matroska file (Qt binding)

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

#include <QObject>
#include <QRecursiveMutex>

#include "common/kax_info.h"
#include "mkvtoolnix-gui/util/runnable.h"

namespace mtx::gui::Util {

class KaxInfoPrivate;
class KaxInfo: public QObject, public ::mtx::kax_info_c {
  Q_OBJECT
protected:
  MTX_DECLARE_PRIVATE(KaxInfoPrivate)

public:
  enum class ScanType {
    StartOfFile,
    Level1Elements,
  };

public:
  KaxInfo();
  KaxInfo(QString const &file_name);
  virtual ~KaxInfo();

  virtual void ui_show_error(std::string const &error) override;
  virtual void ui_show_element_info(int level, std::string const &text, std::optional<int64_t> position, std::optional<int64_t> size, std::optional<int64_t> dataSize) override;
  virtual void ui_show_element(libebml::EbmlElement &e) override;
  virtual void ui_show_progress(int percentage, std::string const &text) override;

  virtual result_e open_and_process_file(std::string const &fileName) override;
  virtual result_e open_and_process_file() override;
  virtual result_e process_file() override;

  virtual QRecursiveMutex &mutex();

  void disableFrameInfo();

public Q_SLOTS:
  virtual void runScan(ScanType type);
  virtual void scanStartOfFile();
  virtual void scanLevel1Elements();
  virtual void abort();

Q_SIGNALS:
  void elementInfoFound(int level, QString const &text, std::optional<int64_t> position, std::optional<int64_t> size, std::optional<int64_t> dataSize);
  void elementFound(int level, libebml::EbmlElement *e, bool readFully);
  void errorFound(const QString &message);
  void progressChanged(int percentage, const QString &text);

  void startOfFileScanStarted();
  void startOfFileScanFinished(mtx::kax_info_c::result_e result);
  void level1ElementsScanStarted();
  void level1ElementsScanFinished(mtx::kax_info_c::result_e result);

protected:
  virtual mtx::kax_info_c::result_e doScanLevel1Elements();
};

}

Q_DECLARE_METATYPE(::mtx::kax_info_c::result_e)
