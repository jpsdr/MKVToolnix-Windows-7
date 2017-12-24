#pragma once

#include "common/common_pch.h"

#include <QThread>

#include "common/qt_kax_info.h"
#include "mkvtoolnix-gui/jobs/job_p.h"

namespace mtx { namespace gui { namespace Jobs {

class InfoJobPrivate: public JobPrivate {
public:
  mtx::gui::Info::InfoConfigPtr config;
  QThread *thread{};
  mtx::qt_kax_info_c *info{};

public:
  explicit InfoJobPrivate(Job::Status pStatus, mtx::gui::Info::InfoConfigPtr const &pConfig);
};

}}}
