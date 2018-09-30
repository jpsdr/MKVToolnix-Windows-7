#pragma once

#include "common/common_pch.h"

#include <QDialog>

namespace mtx { namespace gui { namespace Util {

namespace Ui {
class TextDisplayDialog;
}

class TextDisplayDialog : public QDialog {
  Q_OBJECT;

protected:
  std::unique_ptr<Ui::TextDisplayDialog> ui;

public:
  enum class Format {
    Plain,
    HTML,
    Markdown,
  };

public:
  explicit TextDisplayDialog(QWidget *parent);
  virtual ~TextDisplayDialog();

  TextDisplayDialog &setTitle(QString const &title);
  TextDisplayDialog &setText(QString const &text, Format format);
};

}}}
