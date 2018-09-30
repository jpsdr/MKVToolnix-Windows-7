#pragma once

#include "common/common_pch.h"

#include <QDialog>

namespace mtx { namespace gui { namespace Util {

class TextDisplayDialogPrivate;
class TextDisplayDialog : public QDialog {
  Q_OBJECT;

protected:
  MTX_DECLARE_PRIVATE(TextDisplayDialogPrivate);

  std::unique_ptr<TextDisplayDialogPrivate> const p_ptr;

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

public slots:
  virtual void copyToClipboard();
};

}}}
