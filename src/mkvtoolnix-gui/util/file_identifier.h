#ifndef MTX_MKVTOOLNIX_GUI_UTIL_FILE_IDENTIFIER_H
#define MTX_MKVTOOLNIX_GUI_UTIL_FILE_IDENTIFIER_H

#include "common/common_pch.h"

#include <QStringList>
#include <QVariant>

#include "mkvtoolnix-gui/merge/source_file.h"

class QWidget;

namespace mtx { namespace gui { namespace Util {

class FileIdentifierPrivate;
class FileIdentifier: public QObject {
  Q_OBJECT;

protected:
  Q_DECLARE_PRIVATE(FileIdentifier);

  QScopedPointer<FileIdentifierPrivate> const d_ptr;

  explicit FileIdentifier(FileIdentifierPrivate &d, QWidget *parent);

public:
  FileIdentifier(QWidget *parent = nullptr, QString const &fileName = QString{});
  virtual ~FileIdentifier();

  virtual bool identify();

  virtual QString const &fileName() const;
  virtual void setFileName(QString const &fileName);

  virtual int exitCode() const;
  virtual QStringList const &output() const;

  virtual mtx::gui::Merge::SourceFilePtr const &file() const;

public:
  static void addProbeRangePercentageArg(QStringList &args, double probeRangePercentage);

protected:
  virtual bool parseOutput();
  virtual void parseAttachment(QVariantMap const &obj);
  virtual void parseChapters(QVariantMap const &obj);
  virtual void parseContainer(QVariantMap const &obj);
  virtual void parseGlobalTags(QVariantMap const &obj);
  virtual void parseTrackTags(QVariantMap const &obj);
  virtual void parseTrack(QVariantMap const &obj);
};

}}}

#endif // MTX_MKVTOOLNIX_GUI_UTIL_FILE_IDENTIFIER_H
