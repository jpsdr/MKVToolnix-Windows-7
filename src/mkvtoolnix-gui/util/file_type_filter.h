#ifndef MTX_MKVTOOLNIX_GUI_UTIL_FILE_TYPE_FILTER_H
#define MTX_MKVTOOLNIX_GUI_UTIL_FILE_TYPE_FILTER_H

#include "common/common_pch.h"

#include <QStringList>

namespace mtx { namespace gui { namespace Util {

class FileTypeFilter {
public:
  static QStringList const & get();

public:
  static QStringList s_filter;
};

}}}

#endif // MTX_MKVTOOLNIX_GUI_UTIL_FILE_TYPE_FILTER_H
