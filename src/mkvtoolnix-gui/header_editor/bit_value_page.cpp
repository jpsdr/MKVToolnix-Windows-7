#include "common/common_pch.h"

#include <QLineEdit>
#include <QRegularExpression>

#include "common/qt.h"
#include "mkvtoolnix-gui/header_editor/bit_value_page.h"

namespace mtx::gui::HeaderEditor {

using namespace mtx::gui;

BitValuePage::BitValuePage(Tab &parent,
                           PageBase &topLevelPage,
                           libebml::EbmlMaster &master,
                           libebml::EbmlCallbacks const &callbacks,
                           translatable_string_c const &title,
                           translatable_string_c const &description,
                           unsigned int bitLength)
  : ValuePage{parent, topLevelPage, master, callbacks, ValueType::Binary, title, description}
  , m_originalValue{static_cast<int>(bitLength ? bitLength : 8)}
  , m_bitLength{bitLength}
{
}

BitValuePage::~BitValuePage() {
}

QWidget *
BitValuePage::createInputControl() {
  if (m_element)
    m_originalValue = mtx::bits::value_c{*static_cast<libebml::EbmlBinary *>(m_element)};

  m_leValue = new QLineEdit{this};
  m_leValue->setText(originalValueAsString());
  m_leValue->setClearButtonEnabled(true);

  connect(m_leValue, &QLineEdit::textChanged, this, [this]() { Q_EMIT valueChanged(); });

  return m_leValue;
}

QString
BitValuePage::originalValueAsString()
  const {
  auto value = std::string{};
  auto data  = m_originalValue.data();

  if (data)
    for (auto end = data + m_originalValue.byte_size(); data < end; ++data)
      value += fmt::format("{0:02x}", static_cast<unsigned int>(*data));

  return Q(value);
}

QString
BitValuePage::currentValueAsString()
  const {
  return m_leValue->text();
}

void
BitValuePage::resetValue() {
  m_leValue->setText(originalValueAsString());
}

bool
BitValuePage::validateValue()
  const {
  try {
    valueToBitvalue();
  } catch (...) {
    return false;
  }

  return true;
}

void
BitValuePage::copyValueToElement() {
  auto bitValue = valueToBitvalue();
  static_cast<libebml::EbmlBinary *>(m_element)->CopyBuffer(bitValue.data(), bitValue.byte_size());
}

mtx::bits::value_c
BitValuePage::valueToBitvalue()
  const {
  auto cleanedText = m_leValue->text().replace(QRegularExpression{Q("[^0-9a-fA-F]")}, Q(""));
  auto bitLength   = m_bitLength ? m_bitLength : ((cleanedText.length() + 1) / 2) * 8;

  return { to_utf8(cleanedText), static_cast<unsigned int>(bitLength) };
}

}
