#pragma once

#include "common/common_pch.h"

#include <QWidget>

#include "common/bcp47.h"

class QLabel;

namespace mtx::gui::Util {

namespace Ui {
class LanguageDisplayWidget;
}

class LanguageDisplayWidgetPrivate;
class LanguageDisplayWidget : public QWidget {
  Q_OBJECT

protected:
  MTX_DECLARE_PRIVATE(LanguageDisplayWidgetPrivate)

  std::unique_ptr<LanguageDisplayWidgetPrivate> const p_ptr;

  explicit LanguageDisplayWidget(LanguageDisplayWidgetPrivate &p);

public:
  explicit LanguageDisplayWidget(QWidget *parent);
  ~LanguageDisplayWidget();

  void setAdditionalToolTip(QString const &additionalToolTip);
  void setAdditionalLanguages(QStringList const &additionalLanguages);
  void setAdditionalLanguages(QString const &additionalLanguage);
  void setClearTitle(QString const &title);
  void enableClearingLanguage(bool enable);

  void setLanguage(mtx::bcp47::language_c const &language);
  mtx::bcp47::language_c language() const;

  void registerBuddyLabel(QLabel &buddy);
  virtual bool eventFilter(QObject *watched, QEvent *event) override;

public Q_SLOTS:
  void retranslateUi();

  void clearLanguage();
  void editLanguage();

Q_SIGNALS:
  void languageChanged(mtx::bcp47::language_c const &language);

protected:
  void updateDisplay();
  void updateToolTip();
};

}
