#include "common/common_pch.h"

#include <QDateTimeEdit>
#include <QTimeZone>

#include "common/qt.h"
#include "mkvtoolnix-gui/header_editor/time_value_page.h"
#include "mkvtoolnix-gui/util/string.h"

namespace mtx { namespace gui { namespace HeaderEditor {

using namespace mtx::gui;

TimeValuePage::TimeValuePage(Tab &parent,
                             PageBase &topLevelPage,
                             EbmlMaster &master,
                             EbmlCallbacks const &callbacks,
                             translatable_string_c const &title,
                             translatable_string_c const &description)
  : ValuePage{parent, topLevelPage, master, callbacks, ValueType::String, title, description}
{
}

TimeValuePage::~TimeValuePage() {
}

QWidget *
TimeValuePage::createInputControl() {
  if (m_element)
    m_originalValue = QDateTime::fromMSecsSinceEpoch(static_cast<EbmlDate *>(m_element)->GetEpochDate() * 1000, Qt::UTC).toLocalTime();

  m_dteValue = new QDateTimeEdit{this};
  m_dteValue->setCalendarPopup(true);
  m_dteValue->setDateTime(m_originalValue);
  m_dteValue->setDisplayFormat(Q("yyyy-MM-dd hh:mm:ss"));

  return m_dteValue;
}

QString
TimeValuePage::originalValueAsString()
  const {
  return Util::displayableDate(m_originalValue);
}

QString
TimeValuePage::currentValueAsString()
  const {
  return Util::displayableDate(m_dteValue->dateTime());
}

void
TimeValuePage::resetValue() {
  m_dteValue->setDateTime(m_originalValue);
}

bool
TimeValuePage::validateValue()
  const {
  return m_dteValue->dateTime().isValid();
}

void
TimeValuePage::copyValueToElement() {
  static_cast<EbmlDate *>(m_element)->SetEpochDate(m_dteValue->dateTime().toUTC().toMSecsSinceEpoch() / 1000);
}

}}}
