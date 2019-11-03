#pragma once

#include "common/common_pch.h"

#include <QStringList>

namespace mtx::gui::Util {

class FileTypeFilter {
public:
  static QStringList const &get();
  static void reset();

public:
  static QStringList s_filter;
};

}
