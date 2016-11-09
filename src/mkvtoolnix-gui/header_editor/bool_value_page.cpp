#include "common/common_pch.h"

#include <QComboBox>

#include "common/qt.h"
#include "mkvtoolnix-gui/header_editor/bool_value_page.h"

namespace mtx { namespace gui { namespace HeaderEditor {

using namespace mtx::gui;

BoolValuePage::BoolValuePage(Tab &parent,
                             PageBase &topLevelPage,
                             EbmlMaster &master,
                             EbmlCallbacks const &callbacks,
                             translatable_string_c const &title,
                             translatable_string_c const &description)
  : ValuePage{parent, topLevelPage, master, callbacks, ValueType::Bool, title, description}
{
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

}}}
