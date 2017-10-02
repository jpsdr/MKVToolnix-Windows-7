#pragma once

#include "common/common_pch.h"

#include <QMessageBox>

namespace mtx { namespace gui { namespace Util {

class MessageBox;
class MessageBoxPrivate;

using MessageBoxPtr = std::shared_ptr<MessageBox>;

class MessageBox {
public:
  MessageBox(QWidget *parent);
  virtual ~MessageBox();

  MessageBox &buttons(QMessageBox::StandardButtons pButtons);
  MessageBox &defaultButton(QMessageBox::StandardButton pDefaultButton);
  MessageBox &icon(QMessageBox::Icon pIcon);
  MessageBox &text(QString const &pText);
  MessageBox &title(QString const &pTitle);
  MessageBox &buttonLabel(QMessageBox::StandardButton button, QString const &label);
  MessageBox &onlyOnce(QString const &id);

  QMessageBox::StandardButton exec(boost::optional<QMessageBox::StandardButton> pDefaultButton = boost::optional<QMessageBox::StandardButton>{});

public:
  static MessageBoxPtr question(QWidget *parent);
  static MessageBoxPtr information(QWidget *parent);
  static MessageBoxPtr warning(QWidget *parent);
  static MessageBoxPtr critical(QWidget *parent);

private:
  Q_DISABLE_COPY(MessageBox);
  Q_DECLARE_PRIVATE(MessageBox);

  QScopedPointer<MessageBoxPrivate> const d_ptr;
};

}}}
