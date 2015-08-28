#include "common/common_pch.h"

#include <QDialogButtonBox>
#include <QMessageBox>
#include <QPushButton>

#include "mkvtoolnix-gui/util/message_box.h"

namespace mtx { namespace gui { namespace Util {

MessageBox::MessageBox(QWidget *parent)
  : m_parent{parent}
{
}

MessageBox &
MessageBox::buttons(QMessageBox::StandardButtons pButtons) {
  m_buttons = pButtons;
  return *this;
}

MessageBox &
MessageBox::defaultButton(QMessageBox::StandardButton pDefaultButton) {
  m_defaultButton = pDefaultButton;
  return *this;
}

MessageBox &
MessageBox::icon(QMessageBox::Icon pIcon) {
  m_icon = pIcon;
  return *this;
}

MessageBox &
MessageBox::text(QString const & pText) {
  m_text = pText;
  return *this;
}

MessageBox &
MessageBox::title(QString const & pTitle) {
  m_title = pTitle;
  return *this;
}

MessageBox
MessageBox::question(QWidget *parent) {
  return MessageBox{parent}
    .buttons(QMessageBox::StandardButtons{ QMessageBox::Yes | QMessageBox::No })
    .defaultButton(QMessageBox::No)
    .icon(QMessageBox::Question);
}

MessageBox
MessageBox::information(QWidget *parent) {
  return MessageBox{parent}
    .buttons(QMessageBox::Ok)
    .defaultButton(QMessageBox::NoButton)
    .icon(QMessageBox::Information);
}

MessageBox
MessageBox::warning(QWidget *parent) {
  return MessageBox{parent}
    .buttons(QMessageBox::Ok)
    .defaultButton(QMessageBox::NoButton)
    .icon(QMessageBox::Warning);
}

MessageBox
MessageBox::critical(QWidget *parent) {
  return MessageBox{parent}
    .buttons(QMessageBox::Ok)
    .defaultButton(QMessageBox::NoButton)
    .icon(QMessageBox::Critical);
}

QMessageBox::StandardButton
MessageBox::exec(boost::optional<QMessageBox::StandardButton> pDefaultButton) {
  if (pDefaultButton)
    m_defaultButton = *pDefaultButton;

  QMessageBox msgBox{m_icon, m_title, m_text, QMessageBox::NoButton, m_parent};
  auto buttonBox = msgBox.findChild<QDialogButtonBox *>();
  auto mask      = static_cast<unsigned int>(QMessageBox::FirstButton);

  while (mask <= static_cast<unsigned int>(QMessageBox::LastButton)) {
    auto sb = static_cast<unsigned int>(m_buttons & mask);
    mask  <<= 1;

    if (!sb)
      continue;

    auto button = msgBox.addButton(static_cast<QMessageBox::StandardButton>(sb));

    if (msgBox.defaultButton())
      continue;

    if (   ((m_defaultButton == QMessageBox::NoButton) && (buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole))
        || ((m_defaultButton != QMessageBox::NoButton) && (sb == static_cast<unsigned int>(m_defaultButton))))
      msgBox.setDefaultButton(button);
  }

  // Force labels the user can select no matter what the current style
  // sheet says.
  msgBox.setTextInteractionFlags(Qt::TextBrowserInteraction);

  if (msgBox.exec() == -1)
    return QMessageBox::Cancel;

  return msgBox.standardButton(msgBox.clickedButton());
}

}}}
