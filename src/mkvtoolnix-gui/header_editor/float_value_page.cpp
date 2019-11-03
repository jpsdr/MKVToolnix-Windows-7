#include "common/common_pch.h"

#include <QLineEdit>

#include "common/qt.h"
#include "mkvtoolnix-gui/header_editor/float_value_page.h"

namespace mtx::gui::HeaderEditor {

using namespace mtx::gui;

FloatValuePage::FloatValuePage(Tab &parent,
                               PageBase &topLevelPage,
                               EbmlMaster &master,
                               EbmlCallbacks const &callbacks,
                               translatable_string_c const &title,
                               translatable_string_c const &description)
  : ValuePage{parent, topLevelPage, master, callbacks, ValueType::Float, title, description}
{
}

FloatValuePage::~FloatValuePage() {
}

QWidget *
FloatValuePage::createInputControl() {
  m_leValue = new QLineEdit{this};

  if (m_element) {
    m_originalValue = static_cast<EbmlFloat *>(m_element)->GetValue();
    m_leValue->setText(QString::number(m_originalValue));
  }

  return m_leValue;
}

QString
FloatValuePage::originalValueAsString()
  const {
  if (!m_element)
    return Q("");
  return QString::number(m_originalValue);
}

QString
FloatValuePage::currentValueAsString()
  const {
  return m_leValue->text();
}

void
FloatValuePage::resetValue() {
  m_leValue->setText(QString::number(m_originalValue));
}

bool
FloatValuePage::validateValue()
  const {
  auto ok = false;
  m_leValue->text().toDouble(&ok);
  return ok;
}

void
FloatValuePage::copyValueToElement() {
  static_cast<EbmlFloat *>(m_element)->SetValue(m_leValue->text().toDouble());
}

}
