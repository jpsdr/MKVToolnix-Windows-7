#include "common/common_pch.h"

#include <QLineEdit>

#include "common/qt.h"
#include "mkvtoolnix-gui/header_editor/ascii_string_value_page.h"

namespace mtx::gui::HeaderEditor {

using namespace mtx::gui;

AsciiStringValuePage::AsciiStringValuePage(Tab &parent,
                                           PageBase &topLevelPage,
                                           EbmlMaster &master,
                                           EbmlCallbacks const &callbacks,
                                           translatable_string_c const &title,
                                           translatable_string_c const &description)
  : ValuePage{parent, topLevelPage, master, callbacks, ValueType::AsciiString, title, description}
{
}

AsciiStringValuePage::~AsciiStringValuePage() {
}

QWidget *
AsciiStringValuePage::createInputControl() {
  if (m_element)
    m_originalValue = static_cast<EbmlString *>(m_element)->GetValue();

  m_leValue = new QLineEdit{this};
  m_leValue->setText(Q(m_originalValue));
  m_leValue->setClearButtonEnabled(true);

  connect(m_leValue, &QLineEdit::textChanged, this, [this]() { Q_EMIT valueChanged(); });

  return m_leValue;
}

QString
AsciiStringValuePage::originalValueAsString()
  const {
  return Q(m_originalValue);
}

QString
AsciiStringValuePage::currentValueAsString()
  const {
  return m_leValue->text();
}

void
AsciiStringValuePage::resetValue() {
  m_leValue->setText(Q(m_originalValue));
}

bool
AsciiStringValuePage::validateValue()
  const {
  auto s = to_utf8(m_leValue->text());
  for (auto const &c : s)
    if ((' ' > c) || (126 < c))
      return false;

  return true;
}

void
AsciiStringValuePage::copyValueToElement() {
  static_cast<EbmlString *>(m_element)->SetValue(to_utf8(m_leValue->text()));
}

}
