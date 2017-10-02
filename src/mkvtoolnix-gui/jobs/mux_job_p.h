#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/jobs/job_p.h"

namespace mtx { namespace gui { namespace Jobs {

class MuxJobPrivate: public JobPrivate {
public:
  mtx::gui::Merge::MuxConfigPtr config;
  QProcess process;
  bool aborted{};
  QByteArray bytesRead;
  std::unique_ptr<QTemporaryFile> settingsFile;

public:
  explicit MuxJobPrivate(Job::Status pStatus, mtx::gui::Merge::MuxConfigPtr const &pConfig);
};

}}}
