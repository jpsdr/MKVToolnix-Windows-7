#include "common/common_pch.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>

#include "common/qt.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/settings.h"

namespace mtx { namespace gui { namespace Util {

class MessageBoxPrivate {
  friend class MessageBox;

  QWidget *m_parent{};
  QString m_title, m_text, m_onlyOnceID;
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
  : p_ptr{new MessageBoxPrivate{parent}}
{
}

MessageBox::~MessageBox() {
}

MessageBox &
MessageBox::buttons(QMessageBox::StandardButtons pButtons) {
  p_func()->m_buttons = pButtons;
  return *this;
}

MessageBox &
MessageBox::defaultButton(QMessageBox::StandardButton pDefaultButton) {
  p_func()->m_defaultButton = pDefaultButton;
  return *this;
}

MessageBox &
MessageBox::icon(QMessageBox::Icon pIcon) {
  p_func()->m_icon = pIcon;
  return *this;
}

MessageBox &
MessageBox::text(QString const & pText) {
  p_func()->m_text = pText;
  return *this;
}

MessageBox &
MessageBox::title(QString const & pTitle) {
  p_func()->m_title = pTitle;
  return *this;
}

MessageBox &
MessageBox::buttonLabel(QMessageBox::StandardButton button,
                        QString const &label) {
  p_func()->m_buttonLabels[button] = label;
  return *this;
}

MessageBox &
MessageBox::onlyOnce(QString const &id) {
  p_func()->m_onlyOnceID = id;
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
  auto p = p_func();

  if (pDefaultButton)
    p->m_defaultButton = *pDefaultButton;

  if (!p->m_onlyOnceID.isEmpty()) {
    auto reg          = Util::Settings::registry();
    auto hasBeenShown = reg->value(Q("messageBox/showOnce/%1").arg(p->m_onlyOnceID), false).toBool();

    if (hasBeenShown)
      return p->m_defaultButton;
  }

  QMessageBox msgBox{p->m_icon, p->m_title, p->m_text, QMessageBox::NoButton, p->m_parent};
  auto buttonBox = msgBox.findChild<QDialogButtonBox *>();
  auto mask      = static_cast<unsigned int>(QMessageBox::FirstButton);

  while (mask <= static_cast<unsigned int>(QMessageBox::LastButton)) {
    auto sb = static_cast<unsigned int>(p->m_buttons & mask);
    mask  <<= 1;

    if (!sb)
      continue;

    auto button = msgBox.addButton(static_cast<QMessageBox::StandardButton>(sb));

    if (msgBox.defaultButton())
      continue;

    if (   ((p->m_defaultButton == QMessageBox::NoButton) && (buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole))
        || ((p->m_defaultButton != QMessageBox::NoButton) && (sb == static_cast<unsigned int>(p->m_defaultButton))))
      msgBox.setDefaultButton(button);
  }

  // Force labels the user can select no matter what the current style
  // sheet says.
  msgBox.setTextInteractionFlags(Qt::TextBrowserInteraction);

  for (auto const &standardButton : p->m_buttonLabels.keys()) {
    auto label      = p->m_buttonLabels[standardButton];
    auto pushButton = buttonBox->button(static_cast<QDialogButtonBox::StandardButton>(standardButton));
    if (pushButton && !label.isEmpty())
      pushButton->setText(label);
  }

  if (!p->m_onlyOnceID.isEmpty()) {
    auto checkBox = new QCheckBox{&msgBox};
    checkBox->setText(QY("Don't show this message again."));
    msgBox.setCheckBox(checkBox);
  }

  if (msgBox.exec() == -1)
    return QMessageBox::Cancel;

  if (!p->m_onlyOnceID.isEmpty() && msgBox.checkBox()->isChecked()) {
    auto reg = Util::Settings::registry();
    reg->setValue(Q("messageBox/showOnce/%1").arg(p->m_onlyOnceID), true);
  }

  return msgBox.standardButton(msgBox.clickedButton());
}

}}}
