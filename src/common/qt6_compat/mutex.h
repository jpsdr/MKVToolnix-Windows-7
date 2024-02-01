#pragma once

#include <Qt>

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
# include <QRecursiveMutex>

using MtxQRecursiveMutex = QRecursiveMutex;

# define MTX_QT_RECURSIVE_MUTEX_INIT

#else  // Qt >= 5.14.0
# include <QMutex>

using MtxQRecursiveMutex = QMutex;

# define MTX_QT_RECURSIVE_MUTEX_INIT QMutex::Recursive

#endif  // Qt >= 5.14.0
