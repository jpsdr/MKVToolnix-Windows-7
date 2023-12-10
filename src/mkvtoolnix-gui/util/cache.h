#pragma once

#include "common/common_pch.h"

#include <QHash>
#include <QRecursiveMutex>
#include <QVariant>

namespace mtx::gui::Util {

class ConfigFile;

using CacheProperties = QHash<QString, QVariant>;
using ConfigFilePtr   = std::shared_ptr<ConfigFile>;

class Cache: public QObject {
public:
  static ConfigFilePtr create(QString const &category, QString const &key, CacheProperties const &properties);
  static ConfigFilePtr fetch(QString const &category, QString const &key, CacheProperties const &properties);
  static void remove(QString const &category, QString const &key);

  static void cleanOldCacheFilesForCategory(QString const &category);
  static void cleanOldCacheFiles();
  static void cleanAllCacheFilesForCategory(QString const &category);
  static void cleanAllCacheFiles();

private:
  static QString cacheDirLocation(QString const &category);
  static QString cacheFileName(QString const &category, QString const &key);
  static QRecursiveMutex &cacheDirMutex();
  static QString currentVersionString();
};

}
