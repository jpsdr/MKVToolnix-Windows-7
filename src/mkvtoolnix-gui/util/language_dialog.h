#pragma once

#include "common/common_pch.h"

#include <QDialog>

namespace mtx::bcp47 {
class language_c;
}

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

  void setLanguage(mtx::bcp47::language_c const &language);
  mtx::bcp47::language_c language() const;

  void retranslateUi();

public Q_SLOTS:

protected:
  void setupConnections();
};

}
