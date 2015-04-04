#include "common/common_pch.h"

#include <QAbstractItemView>
#include <QComboBox>

#include "common/qt.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/header_editor/language_value_page.h"

namespace mtx { namespace gui { namespace HeaderEditor {

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
  if (m_element)
    m_originalValue = static_cast<EbmlString *>(m_element)->GetValue();

  auto &descriptions = App::getIso639LanguageDescriptions();
  auto &codes        = App::getIso639Language2Codes();
  m_originalValueIdx = codes.indexOf(Q(m_originalValue));

  if (-1 == m_originalValueIdx)
    m_originalValueIdx = codes.indexOf(Q("und"));

  m_cbValue = new QComboBox{this};
  m_cbValue->setFrame(true);
  m_cbValue->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
  m_cbValue->addItems(descriptions);
  m_cbValue->setCurrentIndex(m_originalValueIdx);
  m_cbValue->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  return m_cbValue;
}

QString
LanguageValuePage::getOriginalValueAsString()
  const {
  return Q(m_originalValue);
}

QString
LanguageValuePage::getCurrentValueAsString()
  const {
  return m_cbValue->currentText();
}

void
LanguageValuePage::resetValue() {
  m_cbValue->setCurrentIndex(m_originalValueIdx);
}

bool
LanguageValuePage::validateValue()
  const {
  return true;
}

void
LanguageValuePage::copyValueToElement() {
  auto &codes = App::getIso639Language2Codes();
  static_cast<EbmlString *>(m_element)->SetValue(to_utf8(codes[ m_cbValue->currentIndex() ]));
}

}}}
