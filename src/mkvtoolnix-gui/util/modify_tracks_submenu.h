#pragma once

#include "common/common_pch.h"

class QAction;
class QMenu;

namespace mtx::gui::Util {

class ModifyTracksSubmenu {
public:
  QAction *m_toggleTrackEnabledFlag{}, *m_toggleDefaultTrackFlag{}, *m_toggleForcedDisplayFlag{}, *m_toggleOriginalFlag{}, *m_toggleCommentaryFlag;
  QAction *m_toggleHearingImpairedFlag{}, *m_toggleVisualImpairedFlag{}, *m_toggleTextDescriptionsFlag{};

public:
  ModifyTracksSubmenu() = default;
  ModifyTracksSubmenu(ModifyTracksSubmenu const &) = delete;
  ModifyTracksSubmenu(ModifyTracksSubmenu &&) = delete;

  void setup(QMenu &subMenu);
  void retranslateUi();
};

}
