#include "common/common_pch.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>

#include "common/checksums/base_fwd.h"
#include "common/qt.h"
#include "common/version.h"
#include "mkvtoolnix-gui/util/cache.h"
#include "mkvtoolnix-gui/util/config_file.h"
#include "mkvtoolnix-gui/util/settings.h"

namespace mtx::gui::Util {

QString
Cache::cacheDirLocation(QString const &category) {
  return Settings::prepareCacheDir(category);
}

QString
Cache::cacheFileName(QString const &category,
                     QString const &key) {
  return QDir::toNativeSeparators(Q("%1/%2").arg(cacheDirLocation(category)).arg(key));
}

QString
Cache::currentVersionString() {
  return Q(get_current_version().to_string());
}

QRecursiveMutex &
Cache::cacheDirMutex() {
  static std::unique_ptr<QRecursiveMutex> s_mutex;

  if (!s_mutex)
    s_mutex.reset(new QRecursiveMutex{});

  return *s_mutex;
}

ConfigFilePtr
Cache::create(QString const &category,
              QString const &key,
              CacheProperties const &properties) {
  QMutexLocker lock{&cacheDirMutex()};

  auto fileName = cacheFileName(category, key);
  auto settings = ConfigFile::create(fileName);

  settings->beginGroup("cacheMetaData");

  for (auto const &propertyKey : properties.keys())
    settings->setValue(propertyKey, properties[propertyKey]);

  settings->setValue("programVersion", currentVersionString());
  settings->setValue("uiLocale",       Settings::get().m_uiLocale);
  settings->endGroup();

  settings->beginGroup("data");

  return settings;
}

ConfigFilePtr
Cache::fetch(QString const &category,
             QString const &key,
             CacheProperties const &properties) {
  QMutexLocker lock{&cacheDirMutex()};

  auto fileName = cacheFileName(category, key);
  auto info     = QFileInfo{fileName};

  if (!info.exists()) {
    qDebug() << "Cache::fetch: false 1; cache file name" << fileName << "cache exists" << info.exists();
    return {};
  }

  auto settings = ConfigFile::open(fileName);
  if (!settings) {
    qDebug() << "Cache::fetch: false 2";
    return {};
  }

  settings->beginGroup("cacheMetaData");

  QStringList mismatches;

  if (settings->value("programVersion").toString() != Q(get_current_version().to_string()))
    mismatches << Q("programVersion");

  if (settings->value("uiLocale").toString() != Settings::get().m_uiLocale)
    mismatches << Q("uiLocale");

  for (auto const &propertyKey : properties.keys())
    if (settings->value(propertyKey) != properties[propertyKey])
      mismatches << propertyKey;

  settings->endGroup();

  if (mismatches.isEmpty()) {
    qDebug() << "Cache::fetch: conditions OK, returning cached content";
    settings->beginGroup(Q("data"));

    return settings;
  }

  settings.reset();
  QFile{fileName}.remove();

  qDebug() << "Cache::fetch: mismatch in cache properties:" << mismatches;

  return {};
}

void
Cache::remove(QString const &category,
              QString const &key) {
  QMutexLocker lock{&cacheDirMutex()};

  QFile{cacheFileName(category, key)}.remove();
}

void
Cache::cleanOldCacheFilesForCategory(QString const &category) {
  auto currentVersion  = currentVersionString();
  auto const &uiLocale = Settings::get().m_uiLocale;
  auto cacheDir        = QDir{cacheDirLocation(category)};
  auto cacheFiles      = cacheDir.entryList(QDir::Files);

  for (auto fileName : cacheFiles) {
    QMutexLocker lock{&cacheDirMutex()};

    fileName  = cacheDir.filePath(fileName);
    auto keep = false;

    auto settings = ConfigFile::open(fileName);
    if (settings) {
      settings->beginGroup("cacheMetaData");
      keep = (settings->value("programVersion").toString() == currentVersion)
          && (settings->value("uiLocale").toString()       == uiLocale);
      settings.reset();
    }

    if (!keep)
      QFile{fileName}.remove();
  }
}

void
Cache::cleanOldCacheFiles() {
  auto cacheDir        = QDir{cacheDirLocation(Q(""))};
  auto cacheCategories = cacheDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

  for (auto category : cacheCategories)
    cleanOldCacheFilesForCategory(category);
}

void
Cache::cleanAllCacheFilesForCategory(QString const &category) {
  QMutexLocker lock{&cacheDirMutex()};

  auto cacheDir   = QDir{cacheDirLocation(category)};
  auto cacheFiles = cacheDir.entryList(QDir::Files);

  for (auto fileName : cacheFiles)
    QFile{cacheDir.filePath(fileName)}.remove();
}

void
Cache::cleanAllCacheFiles() {
  auto cacheDir        = QDir{cacheDirLocation(Q(""))};
  auto cacheCategories = cacheDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

  for (auto category : cacheCategories)
    cleanAllCacheFilesForCategory(category);
}

}
