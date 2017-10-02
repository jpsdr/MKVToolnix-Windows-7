#pragma once

#include "common/common_pch.h"

#include <QString>

#include <matroska/KaxSemantic.h>

using ChaptersPtr = std::shared_ptr<KaxChapters>;
using OptQString  = boost::optional<QString>;
