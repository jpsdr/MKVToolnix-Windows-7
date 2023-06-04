#include "common/common_pch.h"

#include <QCheckBox>
#include <QComboBox>

#include "common/qt.h"
#include "mkvtoolnix-gui/header_editor/bool_value_page.h"

namespace mtx::gui::HeaderEditor {

using namespace mtx::gui;

BoolValuePage::BoolValuePage(Tab &parent,
                             PageBase &topLevelPage,
                             EbmlMaster &master,
                             EbmlCallbacks const &callbacks,
                             translatable_string_c const &title,
                             translatable_string_c const &description)
  : ValuePage{parent, topLevelPage, master, callbacks, ValueType::Bool, title, description}
{
  auto eltWithDefault = std::unique_ptr<EbmlElement>(&callbacks.NewElement());

  if (!dynamic_cast<EbmlUInteger *>(eltWithDefault.get()))
    return;

  auto &eltUInt = static_cast<EbmlUInteger &>(*eltWithDefault.get());
  m_valueIfNotPresent = eltUInt.ValueIsSet() && (eltUInt.GetValue() == 1);
}

BoolValuePage::~BoolValuePage() {
}

QWidget *
BoolValuePage::createInputControl() {
  m_cbValue = new QComboBox{this};
  m_cbValue->addItem(QY("No"));
  m_cbValue->addItem(QY("Yes"));

  if (m_element)
    m_originalValue = std::min<uint64_t>(static_cast<EbmlUInteger *>(m_element)->GetValue(), 1);

  m_cbValue->setCurrentIndex(m_originalValue);

  connect(m_cbValue, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this]() { Q_EMIT valueChanged(); });

  return m_cbValue;
}

QString
BoolValuePage::originalValueAsString()
  const {
  return m_originalValue ? QY("Yes") : QY("No");
}

QString
BoolValuePage::currentValueAsString()
  const {
  return m_cbValue->currentText();
}

bool
BoolValuePage::currentValue(bool valueIfNotPresent)
  const {
  if (!m_present && !m_cbAddOrRemove->isChecked())
    return valueIfNotPresent;

  return m_cbValue->currentIndex() == 1;
}

void
BoolValuePage::resetValue() {
  m_cbValue->setCurrentIndex(m_originalValue);
}

bool
BoolValuePage::validateValue()
  const {
  return true;
}

void
BoolValuePage::copyValueToElement() {
  static_cast<EbmlUInteger *>(m_element)->SetValue(m_cbValue->currentIndex());
}

void
BoolValuePage::toggleFlag() {
  if (!m_present && !m_cbAddOrRemove->isChecked()) {
    m_cbValue->setCurrentIndex(m_valueIfNotPresent ? 1 : 0);
    m_cbAddOrRemove->setChecked(true);
  }

  m_cbValue->setCurrentIndex(1 - m_cbValue->currentIndex());
}

}
