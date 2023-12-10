#pragma once

#include <Qt>

#include <QLibraryInfo>

inline QString
mtxQLibrarylocation(QLibraryInfo::LibraryLocation loc) {
  return QLibraryInfo::path(loc);
}
