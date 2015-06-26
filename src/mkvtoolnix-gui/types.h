#ifndef MTX_MKVTOOLNIX_GUI_UTIL_TYPES_H
#define MTX_MKVTOOLNIX_GUI_UTIL_TYPES_H

#include "common/common_pch.h"

#include <QString>

#include <matroska/KaxSemantic.h>

using ChaptersPtr = std::shared_ptr<KaxChapters>;
using OptQString  = boost::optional<QString>;

#endif  // MTX_MKVTOOLNIX_GUI_UTIL_TYPES_H
