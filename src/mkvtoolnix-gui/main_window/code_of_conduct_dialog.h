#pragma once

#include "common/common_pch.h"

#include <QDialog>

namespace mtx { namespace gui {

namespace Ui {
class CodeOfConductDialog;
}

class CodeOfConductDialog : public QDialog {
  Q_OBJECT;

protected:
  std::unique_ptr<Ui::CodeOfConductDialog> ui;

public:
  explicit CodeOfConductDialog(QWidget *parent);
  virtual ~CodeOfConductDialog();
};

}}
