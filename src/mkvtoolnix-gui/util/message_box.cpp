#include "common/common_pch.h"

#include <QDialogButtonBox>
#include <QMessageBox>
#include <QPushButton>

#include "mkvtoolnix-gui/util/message_box.h"

namespace mtx { namespace gui { namespace Util {

class MessageBoxPrivate {
  friend class MessageBox;

  QWidget *m_parent{};
  QString m_title, m_text;
  QHash<QMessageBox::StandardButton, QString> m_buttonLabels;
  QMessageBox::Icon m_icon{QMessageBox::NoIcon};
  QMessageBox::StandardButtons m_buttons{QMessageBox::Ok};
  QMessageBox::StandardButton m_defaultButton{QMessageBox::Ok};

  explicit MessageBoxPrivate(QWidget *parent)
    : m_parent{parent}
  {
  }
};

MessageBox::MessageBox(QWidget *parent)
  : d_ptr{new MessageBoxPrivate{parent}}
{
}

MessageBox::~MessageBox() {
}

MessageBox &
MessageBox::buttons(QMessageBox::StandardButtons pButtons) {
  Q_D(MessageBox);

  d->m_buttons = pButtons;
  return *this;
}

MessageBox &
MessageBox::defaultButton(QMessageBox::StandardButton pDefaultButton) {
  Q_D(MessageBox);

  d->m_defaultButton = pDefaultButton;
  return *this;
}

MessageBox &
MessageBox::icon(QMessageBox::Icon pIcon) {
  Q_D(MessageBox);

  d->m_icon = pIcon;
  return *this;
}

MessageBox &
MessageBox::text(QString const & pText) {
  Q_D(MessageBox);

  d->m_text = pText;
  return *this;
}

MessageBox &
MessageBox::title(QString const & pTitle) {
  Q_D(MessageBox);

  d->m_title = pTitle;
  return *this;
}

MessageBox &
MessageBox::buttonLabel(QMessageBox::StandardButton button,
                        QString const &label) {
  Q_D(MessageBox);

  d->m_buttonLabels[button] = label;
  return *this;
}

MessageBoxPtr
MessageBox::question(QWidget *parent) {
  auto box = std::make_shared<MessageBox>(parent);
  box->buttons(QMessageBox::StandardButtons{ QMessageBox::Yes | QMessageBox::No })
    .defaultButton(QMessageBox::No)
    .icon(QMessageBox::Question);
  return box;
}

MessageBoxPtr
MessageBox::information(QWidget *parent) {
  auto box = std::make_shared<MessageBox>(parent);
  box->buttons(QMessageBox::Ok)
    .defaultButton(QMessageBox::NoButton)
    .icon(QMessageBox::Information);
  return box;
}

MessageBoxPtr
MessageBox::warning(QWidget *parent) {
  auto box = std::make_shared<MessageBox>(parent);
  box->buttons(QMessageBox::Ok)
    .defaultButton(QMessageBox::NoButton)
    .icon(QMessageBox::Warning);
  return box;
}

MessageBoxPtr
MessageBox::critical(QWidget *parent) {
  auto box = std::make_shared<MessageBox>(parent);
  box->buttons(QMessageBox::Ok)
    .defaultButton(QMessageBox::NoButton)
    .icon(QMessageBox::Critical);
  return box;
}

QMessageBox::StandardButton
MessageBox::exec(boost::optional<QMessageBox::StandardButton> pDefaultButton) {
  Q_D(MessageBox);

  if (pDefaultButton)
    d->m_defaultButton = *pDefaultButton;

  QMessageBox msgBox{d->m_icon, d->m_title, d->m_text, QMessageBox::NoButton, d->m_parent};
  auto buttonBox = msgBox.findChild<QDialogButtonBox *>();
  auto mask      = static_cast<unsigned int>(QMessageBox::FirstButton);

  while (mask <= static_cast<unsigned int>(QMessageBox::LastButton)) {
    auto sb = static_cast<unsigned int>(d->m_buttons & mask);
    mask  <<= 1;

    if (!sb)
      continue;

    auto button = msgBox.addButton(static_cast<QMessageBox::StandardButton>(sb));

    if (msgBox.defaultButton())
      continue;

    if (   ((d->m_defaultButton == QMessageBox::NoButton) && (buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole))
        || ((d->m_defaultButton != QMessageBox::NoButton) && (sb == static_cast<unsigned int>(d->m_defaultButton))))
      msgBox.setDefaultButton(button);
  }

  // Force labels the user can select no matter what the current style
  // sheet says.
  msgBox.setTextInteractionFlags(Qt::TextBrowserInteraction);

  for (auto const &standardButton : d->m_buttonLabels.keys()) {
    auto label      = d->m_buttonLabels[standardButton];
    auto pushButton = buttonBox->button(static_cast<QDialogButtonBox::StandardButton>(standardButton));
    if (pushButton && !label.isEmpty())
      pushButton->setText(label);
  }

  if (msgBox.exec() == -1)
    return QMessageBox::Cancel;

  return msgBox.standardButton(msgBox.clickedButton());
}

}}}
