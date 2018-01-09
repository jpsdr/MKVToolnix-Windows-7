#include "common/common_pch.h"

#include <QObject>

#include "mkvtoolnix-gui/info/initial_scan.h"
#include "mkvtoolnix-gui/util/runnable.h"

namespace mtx { namespace gui { namespace Info {

InitialScan::InitialScan(Util::KaxInfo &info)
  : m_info{info}
{
}

InitialScan::~InitialScan() {
}

void
InitialScan::run() {
  m_info.run();
}

void
InitialScan::abort() {
  m_info.abort();
}

}}}
