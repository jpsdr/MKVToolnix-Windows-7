#pragma once

#include "common/common_pch.h"

#include <QDialog>

class QEvent;

namespace mtx::gui::Util {

class TextDisplayDialogPrivate;
class TextDisplayDialog : public QDialog {
  Q_OBJECT

protected:
  MTX_DECLARE_PRIVATE(TextDisplayDialogPrivate)

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
  TextDisplayDialog &setSaveInfo(QString const &defaultFileName, QString const &filter, QString const &defaultSuffix);

  virtual bool eventFilter(QObject *o, QEvent *e) override;

public Q_SLOTS:
  virtual void copyToClipboard();
  virtual void save();
};

}
