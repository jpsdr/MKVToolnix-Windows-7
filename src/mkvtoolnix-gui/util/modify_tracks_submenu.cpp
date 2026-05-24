#include "common/common_pch.h"

#include <QAction>
#include <QMenu>

#include <matroska/KaxSemantic.h>

#include "common/qt.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/main_window/preferences_dialog.h"
#include "mkvtoolnix-gui/util/modify_tracks_submenu.h"
#include "mkvtoolnix-gui/util/settings.h"

namespace mtx::gui::Util {

void
ModifyTracksSubmenu::setupTrack(QMenu &subMenu) {
  m_toggleTrackEnabledFlag     = new QAction{&subMenu};
  m_toggleDefaultTrackFlag     = new QAction{&subMenu};
  m_toggleForcedDisplayFlag    = new QAction{&subMenu};
  m_toggleOriginalFlag         = new QAction{&subMenu};
  m_toggleCommentaryFlag       = new QAction{&subMenu};
  m_toggleHearingImpairedFlag  = new QAction{&subMenu};
  m_toggleVisualImpairedFlag   = new QAction{&subMenu};
  m_toggleTextDescriptionsFlag = new QAction{&subMenu};

  m_toggleTrackEnabledFlag    ->setShortcut(Q("Ctrl+Alt+F, E"));
  m_toggleDefaultTrackFlag    ->setShortcut(Q("Ctrl+Alt+F, D"));
  m_toggleForcedDisplayFlag   ->setShortcut(Q("Ctrl+Alt+F, F"));
  m_toggleOriginalFlag        ->setShortcut(Q("Ctrl+Alt+F, O"));
  m_toggleCommentaryFlag      ->setShortcut(Q("Ctrl+Alt+F, C"));
  m_toggleHearingImpairedFlag ->setShortcut(Q("Ctrl+Alt+F, H"));
  m_toggleVisualImpairedFlag  ->setShortcut(Q("Ctrl+Alt+F, V"));
  m_toggleTextDescriptionsFlag->setShortcut(Q("Ctrl+Alt+F, T"));

  m_toggleTrackEnabledFlag    ->setData(EBML_ID(libmatroska::KaxTrackFlagEnabled).GetValue());
  m_toggleDefaultTrackFlag    ->setData(EBML_ID(libmatroska::KaxTrackFlagDefault).GetValue());
  m_toggleForcedDisplayFlag   ->setData(EBML_ID(libmatroska::KaxTrackFlagForced).GetValue());
  m_toggleCommentaryFlag      ->setData(EBML_ID(libmatroska::KaxFlagCommentary).GetValue());
  m_toggleOriginalFlag        ->setData(EBML_ID(libmatroska::KaxFlagOriginal).GetValue());
  m_toggleHearingImpairedFlag ->setData(EBML_ID(libmatroska::KaxFlagHearingImpaired).GetValue());
  m_toggleVisualImpairedFlag  ->setData(EBML_ID(libmatroska::KaxFlagVisualImpaired).GetValue());
  m_toggleTextDescriptionsFlag->setData(EBML_ID(libmatroska::KaxFlagTextDescriptions).GetValue());

  subMenu.addAction(m_toggleTrackEnabledFlag);
  subMenu.addAction(m_toggleDefaultTrackFlag);
  subMenu.addAction(m_toggleForcedDisplayFlag);
  subMenu.addAction(m_toggleOriginalFlag);
  subMenu.addAction(m_toggleCommentaryFlag);
  subMenu.addAction(m_toggleHearingImpairedFlag);
  subMenu.addAction(m_toggleVisualImpairedFlag);
  subMenu.addAction(m_toggleTextDescriptionsFlag);
}

void
ModifyTracksSubmenu::setupLanguage(QMenu &subMenu) {
  subMenu.clear();

  auto keyboardShortcutIdx = 0;
  auto structIdx           = -1;

  for (auto const &shortcut : Util::Settings::get().m_languageShortcuts) {
    ++structIdx;

    auto language = mtx::bcp47::language_c::parse(to_utf8(shortcut.m_language));
    if (!language.is_valid())
      continue;

    ++keyboardShortcutIdx;

    auto action = new QAction{&subMenu};
    auto text   = Q(language.format_long());

    if (!shortcut.m_trackName.isEmpty())
      text = Q("%1; %2: %3").arg(text).arg(QY("track name")).arg(shortcut.m_trackName);

    action->setData(structIdx);
    action->setText(text);

    if (keyboardShortcutIdx <= 10)
      action->setShortcut(Q("Ctrl+Alt+A, %1").arg(keyboardShortcutIdx % 10));

    subMenu.addAction(action);

    connect(action, &QAction::triggered, this, &ModifyTracksSubmenu::languageChangeTriggered);
  }

  if (keyboardShortcutIdx)
    subMenu.addSeparator();

  m_configureLanguages = new QAction{&subMenu};
  subMenu.addAction(m_configureLanguages);

  connect(m_configureLanguages, &QAction::triggered, this, []() { MainWindow::get()->editPreferencesAndShowPage(PreferencesDialog::Page::Languages); });

  retranslateUi();
}

void
ModifyTracksSubmenu::retranslateUi() {
  if (m_configureLanguages)
    m_configureLanguages->setText(QY("&Configure available languages"));

  if (!m_toggleTrackEnabledFlag)
    return;

  m_toggleTrackEnabledFlag    ->setText(QY("Toggle \"track &enabled\" flag"));
  m_toggleDefaultTrackFlag    ->setText(QY("Toggle \"&default track\" flag"));
  m_toggleForcedDisplayFlag   ->setText(QY("Toggle \"&forced display\" flag"));
  m_toggleOriginalFlag        ->setText(QY("Toggle \"&original\" flag"));
  m_toggleCommentaryFlag      ->setText(QY("Toggle \"&commentary\" flag"));
  m_toggleHearingImpairedFlag ->setText(QY("Toggle \"&hearing impaired\" flag"));
  m_toggleVisualImpairedFlag  ->setText(QY("Toggle \"&visual impaired\" flag"));
  m_toggleTextDescriptionsFlag->setText(QY("Toggle \"&text descriptions\" flag"));
}

void
ModifyTracksSubmenu::languageChangeTriggered() {
  auto action = dynamic_cast<QAction *>(sender());
  if (!action)
    return;

  auto const &shortcuts = Util::Settings::get().m_languageShortcuts;
  auto structIdx        = action->data().toInt();

  if ((structIdx < 0) || (structIdx >= shortcuts.size()))
    return;

  auto const &shortcut = shortcuts.at(structIdx);

  if (!shortcut.m_language.isEmpty())
    Q_EMIT languageChangeRequested(shortcut.m_language, shortcut.m_trackName);
}

}
