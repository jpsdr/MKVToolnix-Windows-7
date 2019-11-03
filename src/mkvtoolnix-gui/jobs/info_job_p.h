#pragma once

#include "common/common_pch.h"

#include <QThread>

#include "mkvtoolnix-gui/jobs/job_p.h"
#include "mkvtoolnix-gui/util/kax_info.h"

namespace mtx::gui::Jobs {

class InfoJobPrivate: public JobPrivate {
public:
  mtx::gui::Info::InfoConfigPtr config;
  QThread *thread{};
  Util::KaxInfo *info{};

public:
  explicit InfoJobPrivate(Job::Status pStatus, mtx::gui::Info::InfoConfigPtr const &pConfig);
};

}
