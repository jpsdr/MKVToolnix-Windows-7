#pragma once

#include "common/common_pch.h"

#include <QWidget>

#include "common/bcp47.h"

class QComboBox;
class QLineEdit;
class QRegularExpression;

namespace mtx::gui::Util {

namespace Ui {
class LanguageWidget;
}

class LanguageWidgetPrivate;
class LanguageWidget : public QWidget {
  Q_OBJECT

protected:
  MTX_DECLARE_PRIVATE(LanguageWidgetPrivate)

  std::unique_ptr<LanguageWidgetPrivate> const p_ptr;

  explicit LanguageWidget(LanguageWidgetPrivate &p);

public:
  explicit LanguageWidget(QWidget *parent);
  ~LanguageWidget();

  void retranslateUi();

  void setAdditionalLanguages(QStringList const &additionalLanguages);

  void setLanguage(mtx::bcp47::language_c const &language);
  mtx::bcp47::language_c language() const;

public Q_SLOTS:
  void updateFromFreeForm();
  void updateFromComponents();

  void addExtendedSubtagRowAndUpdateLayout();
  void addVariantRowAndUpdateLayout();
  void addPrivateUseRowAndUpdateLayout();

  void removeRowForClickedButton();

  void maybeEnableAddExtendedSubtagButton();

Q_SIGNALS:
  void tagValidityChanged(bool isValid);

protected:
  void setupConnections();
  void setupFreeFormAndComponentControls();
  void connectComponentWidgetChange(QWidget *widget);

  void setupExtendedSubtagsComboBox(QComboBox &comboBox);
  void setupScriptComboBox();
  void setupVariantComboBox(QComboBox &comboBox);

  void reinitializeLanguageComboBox();

  QVector<QWidget *> allComponentWidgets();
  QVector<QWidget *> allComponentWidgetsMatchingName(QRegularExpression const &matcher);

  mtx::bcp47::language_c languageTagFromComponents();
  void setComponentsFromLanguageTag(mtx::bcp47::language_c const &tag);

  void setStatusFromLanguageTag(mtx::bcp47::language_c const &tag);
  void setMultipleWidgetsTexts(QString const &objectNamePrefix, std::vector<std::string> const &values);
  void setWidgetText(QWidget &widget, QString const &text);

  QWidget *addRowItem(QString const &type, std::function<QWidget *(int)> const &createWidget);
  QWidget *addExtendedSubtagRow();
  QWidget *addVariantRow();
  QWidget *addPrivateUseRow();

  int componentGridRowForWidget(QWidget *widget);
  void removeRowItems(QString const &namePrefix);
  void createInitialComponentWidgetList();
  void createGridLayoutFromComponentWidgetList();
};

}
