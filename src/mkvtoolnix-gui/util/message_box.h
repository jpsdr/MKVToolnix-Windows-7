#ifndef MTX_MKVTOOLNIX_GUI_UTIL_MESSAGE_BOX_H
#define MTX_MKVTOOLNIX_GUI_UTIL_MESSAGE_BOX_H

#include "common/common_pch.h"

#include <QMessageBox>

namespace mtx { namespace gui { namespace Util {

class MessageBox {
public:
  static QMessageBox::StandardButton question(QWidget *parent, QString const &title, QString const &text,
                                              QMessageBox::StandardButtons buttons = QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No),
                                              QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
  static QMessageBox::StandardButton information(QWidget *parent, QString const &title, QString const &text,
                                                 QMessageBox::StandardButtons buttons = QMessageBox::Ok,
                                                 QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
  static QMessageBox::StandardButton warning(QWidget *parent, QString const &title, QString const &text,
                                             QMessageBox::StandardButtons buttons = QMessageBox::Ok,
                                             QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
  static QMessageBox::StandardButton critical(QWidget *parent, QString const &title, QString const &text,
                                              QMessageBox::StandardButtons buttons = QMessageBox::Ok,
                                              QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_UTIL_MESSAGE_BOX_H
