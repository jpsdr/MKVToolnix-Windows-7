#ifndef MTX_MKVTOOLNIX_GUI_UTIL_MESSAGE_BOX_H
#define MTX_MKVTOOLNIX_GUI_UTIL_MESSAGE_BOX_H

#include "common/common_pch.h"

#include <QMessageBox>

namespace mtx { namespace gui { namespace Util {

class MessageBox {
protected:
  QWidget *m_parent{};
  QString m_title, m_text;
  QMessageBox::Icon m_icon{QMessageBox::NoIcon};
  QMessageBox::StandardButtons m_buttons{QMessageBox::Ok};
  QMessageBox::StandardButton m_defaultButton{QMessageBox::Ok};

public:
  MessageBox(QWidget *parent);

  MessageBox &buttons(QMessageBox::StandardButtons pButtons);
  MessageBox &defaultButton(QMessageBox::StandardButton pDefaultButton);
  MessageBox &icon(QMessageBox::Icon pIcon);
  MessageBox &text(QString const &pText);
  MessageBox &title(QString const &pTitle);

  QMessageBox::StandardButton exec(boost::optional<QMessageBox::StandardButton> pDefaultButton = boost::optional<QMessageBox::StandardButton>{});

public:
  static MessageBox question(QWidget *parent);
  static MessageBox information(QWidget *parent);
  static MessageBox warning(QWidget *parent);
  static MessageBox critical(QWidget *parent);
};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_UTIL_MESSAGE_BOX_H
