#include "common/common_pch.h"

#include <QDebug>
#include <QObject>

#include "mkvtoolnix-gui/info/initial_scan.h"
#include "mkvtoolnix-gui/util/runnable.h"

namespace mtx { namespace gui { namespace Info {

InitialScan::InitialScan(Util::KaxInfo &info,
                         Util::KaxInfo::ScanType type)
  : m_info{info}
  , m_type{type}
{
}

InitialScan::~InitialScan() {
}

void
InitialScan::run() {
  m_info.runScan(m_type);
}

void
InitialScan::abort() {
  m_info.abort();
}

}}}
