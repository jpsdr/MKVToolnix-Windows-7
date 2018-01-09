#pragma once

#include "common/common_pch.h"

#include <QRunnable>

namespace mtx { namespace gui { namespace Util {

class Runnable : public QRunnable {
public:
  virtual void abort() = 0;
};

}}}
