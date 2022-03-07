#pragma once

#include "common/common_pch.h"

#include <QObject>

class QAction;
class QMenu;

namespace mtx::gui::Util {

class ModifyTracksSubmenu: public QObject {
  Q_OBJECT

public:
  QAction *m_toggleTrackEnabledFlag{}, *m_toggleDefaultTrackFlag{}, *m_toggleForcedDisplayFlag{}, *m_toggleOriginalFlag{}, *m_toggleCommentaryFlag;
  QAction *m_toggleHearingImpairedFlag{}, *m_toggleVisualImpairedFlag{}, *m_toggleTextDescriptionsFlag{};
  QAction *m_configureLanguages{};

public:
  ModifyTracksSubmenu() = default;
  ModifyTracksSubmenu(ModifyTracksSubmenu const &) = delete;
  ModifyTracksSubmenu(ModifyTracksSubmenu &&) = delete;

  void setupTrack(QMenu &subMenu);
  void setupLanguage(QMenu &subMenu);
  void retranslateUi();

Q_SIGNALS:
  void languageChangeRequested(QString const &language, QString const &trackName);

public Q_SLOTS:
  void languageChangeTriggered();
};

}
