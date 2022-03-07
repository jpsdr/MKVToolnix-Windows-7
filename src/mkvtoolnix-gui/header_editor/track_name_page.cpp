#include "common/common_pch.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>

#include "mkvtoolnix-gui/header_editor/track_name_page.h"
#include "mkvtoolnix-gui/main_window/main_window.h"

namespace mtx::gui::HeaderEditor {

using namespace mtx::gui;

TrackNamePage::TrackNamePage(Tab &parent,
                             PageBase &topLevelPage,
                             EbmlMaster &master,
                             EbmlCallbacks const &callbacks,
                             translatable_string_c const &title,
                             translatable_string_c const &description)
  : ValuePage{parent, topLevelPage, master, callbacks, ValueType::String, title, description}
  , m_trackType{FindChildValue<libmatroska::KaxTrackType>(m_master)}
{
}

TrackNamePage::~TrackNamePage() {
}

QWidget *
TrackNamePage::createInputControl() {
  if (m_element)
    m_originalValue = Q(static_cast<EbmlUnicodeString *>(m_element)->GetValue());

  m_cbTrackName = new QComboBox{this};
  m_cbTrackName->setEditable(true);
  m_cbTrackName->setCurrentText(m_originalValue);
  m_cbTrackName->lineEdit()->setClearButtonEnabled(true);

  connect(MainWindow::get(), &MainWindow::preferencesChanged, this, &TrackNamePage::setupPredefinedTrackNames);

  setupPredefinedTrackNames();

  return m_cbTrackName;
}

QString
TrackNamePage::originalValueAsString()
  const {
  return m_originalValue;
}

QString
TrackNamePage::currentValueAsString()
  const {
  return m_cbTrackName->currentText();
}

void
TrackNamePage::resetValue() {
  m_cbTrackName->setCurrentText(m_originalValue);
}

bool
TrackNamePage::validateValue()
  const {
  return true;
}

void
TrackNamePage::copyValueToElement() {
  static_cast<EbmlUnicodeString *>(m_element)->SetValue(to_wide(m_cbTrackName->currentText()));
}

void
TrackNamePage::setupPredefinedTrackNames() {
  auto name      = m_cbTrackName->currentText();
  auto &settings = Util::Settings::get();
  auto &list     = track_audio == m_trackType ? settings.m_mergePredefinedAudioTrackNames
                 : track_video == m_trackType ? settings.m_mergePredefinedVideoTrackNames
                 :                              settings.m_mergePredefinedSubtitleTrackNames;

  m_cbTrackName->clear();
  m_cbTrackName->addItems(list);
  m_cbTrackName->setCurrentText(name);
}

void
TrackNamePage::setString(QString const &value) {
  if (!m_present && !m_cbAddOrRemove->isChecked())
    m_cbAddOrRemove->setChecked(true);

  m_cbTrackName->setCurrentText(value);
}

}
