#pragma once

#include "common/common_pch.h"

#include <QStringList>
#include <QVariant>

#include "common/qt.h"
#include "mkvtoolnix-gui/merge/source_file.h"

namespace mtx::gui::Util {

class FileIdentifierPrivate;
class FileIdentifier: public QObject {
  Q_OBJECT

protected:
  MTX_DECLARE_PRIVATE(FileIdentifierPrivate)

  std::unique_ptr<FileIdentifierPrivate> const p_ptr;

  explicit FileIdentifier(FileIdentifierPrivate &p);

public:
  FileIdentifier(QString const &fileName = QString{});
  virtual ~FileIdentifier();

  virtual bool identify();

  virtual QString const &fileName() const;
  virtual void setFileName(QString const &fileName);

  virtual int exitCode() const;
  virtual QStringList const &output() const;

  virtual mtx::gui::Merge::SourceFilePtr const &file() const;

  virtual QString const &errorTitle() const;
  virtual QString const &errorText() const;

public:
  static QStringList probeRangePercentageArgs(double probeRangePercentage);
  static void cleanAllCacheFiles();

protected:
  virtual bool parseOutput();
  virtual void parseAttachment(QVariantMap const &obj);
  virtual void parseChapters(QVariantMap const &obj);
  virtual void parseContainer(QVariantMap const &obj);
  virtual void parseGlobalTags(QVariantMap const &obj);
  virtual void parseTrackTags(QVariantMap const &obj);
  virtual void parseTrack(QVariantMap const &obj);

  virtual void setDefaults();

  virtual void setError(QString const &errorTitle, QString const &errorText);

  virtual QString cacheKey() const;
  virtual QHash<QString, QVariant> cacheProperties() const;
  virtual void storeResultInCache() const;
  virtual bool retrieveResultFromCache();

protected:
  static QString cacheCategory();
};

}
