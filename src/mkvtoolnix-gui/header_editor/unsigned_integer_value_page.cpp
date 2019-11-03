#include "common/common_pch.h"

#include <QLineEdit>

#include "common/qt.h"
#include "mkvtoolnix-gui/header_editor/unsigned_integer_value_page.h"

namespace mtx::gui::HeaderEditor {

using namespace mtx::gui;

UnsignedIntegerValuePage::UnsignedIntegerValuePage(Tab &parent,
                                                   PageBase &topLevelPage,
                                                   EbmlMaster &master,
                                                   EbmlCallbacks const &callbacks,
                                                   translatable_string_c const &title,
                                                   translatable_string_c const &description)
  : ValuePage{parent, topLevelPage, master, callbacks, ValueType::UnsignedInteger, title, description}
{
}

UnsignedIntegerValuePage::~UnsignedIntegerValuePage() {
}

QWidget *
UnsignedIntegerValuePage::createInputControl() {
  m_leValue = new QLineEdit{this};

  if (m_element) {
    m_originalValue = static_cast<EbmlUInteger *>(m_element)->GetValue();
    m_leValue->setText(QString::number(m_originalValue));
  }

  return m_leValue;
}

QString
UnsignedIntegerValuePage::originalValueAsString()
  const {
  if (!m_element)
    return Q("");
  return QString::number(m_originalValue);
}

QString
UnsignedIntegerValuePage::currentValueAsString()
  const {
  return m_leValue->text();
}

void
UnsignedIntegerValuePage::resetValue() {
  m_leValue->setText(QString::number(m_originalValue));
}

bool
UnsignedIntegerValuePage::validateValue()
  const {
  auto ok = false;
  m_leValue->text().toULongLong(&ok);
  return ok;
}

void
UnsignedIntegerValuePage::copyValueToElement() {
  static_cast<EbmlUInteger *>(m_element)->SetValue(m_leValue->text().toULongLong());
}

}
