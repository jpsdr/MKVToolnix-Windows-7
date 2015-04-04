#include "common/common_pch.h"

#include <QLineEdit>

#include "common/qt.h"
#include "mkvtoolnix-gui/header_editor/string_value_page.h"

namespace mtx { namespace gui { namespace HeaderEditor {

using namespace mtx::gui;

StringValuePage::StringValuePage(Tab &parent,
                                 PageBase &topLevelPage,
                                 EbmlMaster &master,
                                 EbmlCallbacks const &callbacks,
                                 translatable_string_c const &title,
                                 translatable_string_c const &description)
  : ValuePage{parent, topLevelPage, master, callbacks, ValueType::String, title, description}
{
}

StringValuePage::~StringValuePage() {
}

QWidget *
StringValuePage::createInputControl() {
  if (m_element)
    m_originalValue = Q(static_cast<EbmlUnicodeString *>(m_element)->GetValue());

  m_leText = new QLineEdit{this};
  m_leText->setText(m_originalValue);

  return m_leText;
}

QString
StringValuePage::getOriginalValueAsString()
  const {
  return m_originalValue;
}

QString
StringValuePage::getCurrentValueAsString()
  const {
  return m_leText->text();
}

void
StringValuePage::resetValue() {
  m_leText->setText(m_originalValue);
}

bool
StringValuePage::validateValue()
  const {
  return true;
}

void
StringValuePage::copyValueToElement() {
  static_cast<EbmlUnicodeString *>(m_element)->SetValue(to_wide(m_leText->text()));
}

}}}
