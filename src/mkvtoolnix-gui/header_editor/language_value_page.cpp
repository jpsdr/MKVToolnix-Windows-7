#include "common/common_pch.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>

#include "common/iso639.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/header_editor/language_value_page.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/language_combo_box.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx::gui::HeaderEditor {

using namespace mtx::gui;

LanguageValuePage::LanguageValuePage(Tab &parent,
                                     PageBase &topLevelPage,
                                     EbmlMaster &master,
                                     EbmlCallbacks const &callbacks,
                                     translatable_string_c const &title,
                                     translatable_string_c const &description)
  : ValuePage{parent, topLevelPage, master, callbacks, ValueType::AsciiString, title, description}
{
}

LanguageValuePage::~LanguageValuePage() {
}

QWidget *
LanguageValuePage::createInputControl() {
  m_originalValue   = m_element ? static_cast<EbmlString *>(m_element)->GetValue() : "eng";
  auto qOriginal    = Q(m_originalValue);
  auto languageOnly = qOriginal.mid(0, qOriginal.indexOf(Q('-')));
  auto languageOpt  = mtx::iso639::look_up(to_utf8(languageOnly), true);
  auto currentValue = languageOpt ? languageOpt->alpha_3_code : m_originalValue;

  m_cbValue = new Util::LanguageComboBox{this};
  m_cbValue->setFrame(true);
  m_cbValue->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);

  m_cbValue->setAdditionalItems(Q(currentValue))
    .setup()
    .setCurrentByData(QStringList{} << Q(currentValue) << Q("und"));

  connect(MainWindow::get(), &MainWindow::preferencesChanged, m_cbValue, &Util::ComboBoxBase::reInitialize);
  connect(m_cbValue, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this]() { Q_EMIT valueChanged(); });

  return m_cbValue;
}

QString
LanguageValuePage::originalValueAsString()
  const {
  return Q(m_originalValue);
}

QString
LanguageValuePage::currentValueAsString()
  const {
  return m_cbValue->currentData().toString();
}

mtx::bcp47::language_c
LanguageValuePage::currentValue(mtx::bcp47::language_c const &valueIfNotPresent)
  const {
  if (!m_present && !m_cbAddOrRemove->isChecked())
    return valueIfNotPresent;

  return mtx::bcp47::language_c::parse(to_utf8(m_cbValue->currentData().toString()));
}

void
LanguageValuePage::resetValue() {
  m_cbValue->setCurrentByData(Q(m_originalValue));
}

bool
LanguageValuePage::validateValue()
  const {
  return true;
}

void
LanguageValuePage::copyValueToElement() {
  static_cast<EbmlString *>(m_element)->SetValue(to_utf8(currentValueAsString()));
}

void
LanguageValuePage::setLanguage(mtx::bcp47::language_c const &parsedLanguage) {
  if (!m_present && !m_cbAddOrRemove->isChecked())
    m_cbAddOrRemove->setChecked(true);

  m_cbValue->setCurrentByData(QStringList{} << Q(parsedLanguage.get_closest_iso639_2_alpha_3_code()));
}

}
