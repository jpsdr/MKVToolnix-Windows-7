#pragma once

#include "common/common_pch.h"

#include <QModelIndex>

class QTreeView;

namespace mtx::gui::Util {

void expandCollapseAll(QTreeView *view, bool expand, QModelIndex const &parentIdx = {});

}
