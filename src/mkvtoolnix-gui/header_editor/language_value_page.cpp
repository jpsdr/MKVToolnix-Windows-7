#include "common/common_pch.h"

#include <QAbstractItemView>
#include <QComboBox>

#include "common/qt.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/header_editor/language_value_page.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/language_combo_box.h"
#include "mkvtoolnix-gui/util/util.h"

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
  m_originalValue = m_element ? static_cast<EbmlString *>(m_element)->GetValue() : "eng";

  m_cbValue = new Util::LanguageComboBox{this};
  m_cbValue->setFrame(true);
  m_cbValue->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);

  m_cbValue->setup().setCurrentByData(QStringList{} << Q(m_originalValue) << Q("und"));
  m_originalValueIdx = m_cbValue->currentIndex();

  connect(MainWindow::get(), &MainWindow::preferencesChanged, m_cbValue, &Util::ComboBoxBase::reInitialize);

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
  static_cast<EbmlString *>(m_element)->SetValue(to_utf8(currentValueAsString()));
}

}}}
