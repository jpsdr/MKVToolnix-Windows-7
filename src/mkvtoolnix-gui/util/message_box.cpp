#include "common/common_pch.h"

#include <QDialogButtonBox>
#include <QMessageBox>
#include <QPushButton>

#include "mkvtoolnix-gui/util/message_box.h"

namespace mtx { namespace gui { namespace Util {

static QMessageBox::StandardButton
showMessageBox(QWidget *parent,
               QMessageBox::Icon icon,
               QString const &title,
               QString const &text,
               QMessageBox::StandardButtons buttons,
               QMessageBox::StandardButton defaultButton) {
  QMessageBox msgBox{icon, title, text, QMessageBox::NoButton, parent};
  auto buttonBox = msgBox.findChild<QDialogButtonBox *>();
  auto mask      = static_cast<unsigned int>(QMessageBox::FirstButton);

  while (mask <= static_cast<unsigned int>(QMessageBox::LastButton)) {
    auto sb = static_cast<unsigned int>(buttons & mask);
    mask  <<= 1;

    if (!sb)
      continue;

    auto button = msgBox.addButton(static_cast<QMessageBox::StandardButton>(sb));

    if (msgBox.defaultButton())
      continue;

    if (   ((defaultButton == QMessageBox::NoButton) && (buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole))
        || ((defaultButton != QMessageBox::NoButton) && (sb == static_cast<unsigned int>(defaultButton))))
      msgBox.setDefaultButton(button);
  }

  // Force labels the user can select no matter what the current style
  // sheet says.
  msgBox.setTextInteractionFlags(Qt::TextBrowserInteraction);

  if (msgBox.exec() == -1)
    return QMessageBox::Cancel;

  return msgBox.standardButton(msgBox.clickedButton());
}

QMessageBox::StandardButton
MessageBox::question(QWidget *parent,
                     QString const &title,
                     QString const &text,
                     QMessageBox::StandardButtons buttons,
                     QMessageBox::StandardButton defaultButton) {
  return showMessageBox(parent, QMessageBox::Question, title, text, buttons, defaultButton);
}

QMessageBox::StandardButton
MessageBox::information(QWidget *parent,
                        QString const &title,
                        QString const &text,
                        QMessageBox::StandardButtons buttons,
                        QMessageBox::StandardButton defaultButton) {
  return showMessageBox(parent, QMessageBox::Information, title, text, buttons, defaultButton);
}

QMessageBox::StandardButton
MessageBox::warning(QWidget *parent,
                    QString const &title,
                    QString const &text,
                    QMessageBox::StandardButtons buttons,
                    QMessageBox::StandardButton defaultButton) {
  return showMessageBox(parent, QMessageBox::Warning, title, text, buttons, defaultButton);
}

QMessageBox::StandardButton
MessageBox::critical(QWidget *parent,
                     QString const &title,
                     QString const &text,
                     QMessageBox::StandardButtons buttons,
                     QMessageBox::StandardButton defaultButton) {
  return showMessageBox(parent, QMessageBox::Critical, title, text, buttons, defaultButton);
}

}}}
