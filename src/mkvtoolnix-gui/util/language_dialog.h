#pragma once

#include "common/common_pch.h"

#include <QDialog>

namespace mtx::gui::Util {

namespace Ui {
class LanguageDialog;
}

class LanguageDialogPrivate;
class LanguageDialog : public QDialog {
  Q_OBJECT

protected:
  MTX_DECLARE_PRIVATE(LanguageDialogPrivate)

  std::unique_ptr<LanguageDialogPrivate> const p_ptr;

  explicit LanguageDialog(LanguageDialogPrivate &p);

public:
  explicit LanguageDialog(QWidget *parent);
  ~LanguageDialog();

  void setAdditionalLanguages(QStringList const &additionalLanguages);

  void setLanguage(QString const &language);
  QString language() const;

  void retranslateUi();

public Q_SLOTS:

protected:
  void setupConnections();
};

}
