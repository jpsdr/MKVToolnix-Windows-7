#include "common/common_pch.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>

#include "common/iso639.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/header_editor/language_ietf_value_page.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/language_display_widget.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx::gui::HeaderEditor {

using namespace mtx::gui;

LanguageIETFValuePage::LanguageIETFValuePage(Tab &parent,
                                             PageBase &topLevelPage,
                                             libebml::EbmlMaster &master,
                                             libebml::EbmlCallbacks const &callbacks,
                                             translatable_string_c const &title,
                                             translatable_string_c const &description)
  : ValuePage{parent, topLevelPage, master, callbacks, ValueType::AsciiString, title, description}
{
}

LanguageIETFValuePage::~LanguageIETFValuePage() {
}

QWidget *
LanguageIETFValuePage::createInputControl() {
  m_originalValue     = m_element ? static_cast<libebml::EbmlString *>(m_element)->GetValue() : ""s;
  auto parsedLanguage = mtx::bcp47::language_c::parse(m_originalValue);
  auto currentValue   = parsedLanguage.is_valid() ? parsedLanguage.format() : m_originalValue;

  m_ldwValue          = new Util::LanguageDisplayWidget{this};

  m_ldwValue->setLanguage(parsedLanguage);

  m_ldwValue->registerBuddyLabel(*m_lValueLabel);

  connect(m_ldwValue, &Util::LanguageDisplayWidget::languageChanged, this, [this]() { Q_EMIT valueChanged(); });

  return m_ldwValue;
}

QString
LanguageIETFValuePage::originalValueAsString()
  const {
  return Q(m_originalValue);
}

QString
LanguageIETFValuePage::currentValueAsString()
  const {
  return Q(m_ldwValue->language().format());
}

mtx::bcp47::language_c
LanguageIETFValuePage::currentValue(mtx::bcp47::language_c const &valueIfNotPresent)
  const {
  if (!m_present && !m_cbAddOrRemove->isChecked())
    return valueIfNotPresent;

  return m_ldwValue->language();
}

void
LanguageIETFValuePage::resetValue() {
  m_ldwValue->setLanguage(mtx::bcp47::language_c::parse(m_originalValue));
}

bool
LanguageIETFValuePage::validateValue()
  const {
  return m_ldwValue->language().is_valid();
}

void
LanguageIETFValuePage::copyValueToElement() {
  static_cast<libebml::EbmlString *>(m_element)->SetValue(to_utf8(currentValueAsString()));
}

void
LanguageIETFValuePage::retranslateUi() {
  ValuePage::retranslateUi();

  if (m_lValueLabel)
    m_lValueLabel->setText(QY("New &value:"));
}

void
LanguageIETFValuePage::setLanguage(mtx::bcp47::language_c const &parsedLanguage) {
  if (!m_present && !m_cbAddOrRemove->isChecked())
    m_cbAddOrRemove->setChecked(true);

  m_ldwValue->setLanguage(parsedLanguage);
}

void
LanguageIETFValuePage::deriveFromLegacyIfNotPresent(QString const &languageStr) {
  if (m_present)
    return;

  auto language = mtx::bcp47::language_c::parse(to_utf8(languageStr));
  if (!language.is_valid())
    language = mtx::bcp47::language_c::parse("eng");

  setLanguage(language);
}

}
