#pragma once

#include "common/common_pch.h"

#include <QModelIndex>

class QTreeView;

namespace mtx { namespace gui { namespace Util {

void expandCollapseAll(QTreeView *view, bool expand, QModelIndex const &parentIdx = {});

}}}
