#pragma once

#include <Qt>

#include <QLibraryInfo>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)

inline QString
mtxQLibrarylocation(QLibraryInfo::LibraryLocation loc) {
  return QLibraryInfo::path(loc);
}

#else  // Qt >= 6

inline QString
mtxQLibrarylocation(QLibraryInfo::LibraryLocation loc) {
  return QLibraryInfo::location(loc);
}

#endif  // Qt >= 6
