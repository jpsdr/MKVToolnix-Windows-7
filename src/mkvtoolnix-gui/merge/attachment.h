#pragma once

#include "common/common_pch.h"

#include "common/qt.h"
#include "mkvtoolnix-gui/merge/mux_config.h"

#include <QString>

class QStringList;

namespace mtx { namespace gui {

namespace Util {
class ConfigFile;
}

namespace Merge {

class Attachment;
using AttachmentPtr = std::shared_ptr<Attachment>;

class Attachment {
public:
  enum Style {
    ToAllFiles = 1,
    ToFirstFile,
    StyleMax = ToFirstFile,
    StyleMin = ToAllFiles
  };

  QString m_fileName, m_name, m_description, m_MIMEType;
  Style m_style;

public:
  explicit Attachment(QString const &fileName = QString{""});
  virtual ~Attachment();

  virtual void saveSettings(Util::ConfigFile &settings) const;
  virtual void loadSettings(MuxConfig::Loader &l);
  virtual void guessMIMEType();

  void buildMkvmergeOptions(QStringList &options) const;
};

}}}
