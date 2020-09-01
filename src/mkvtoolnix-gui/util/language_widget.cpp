#include "common/common_pch.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QLineEdit>
#include <QRadioButton>
#include <QRegularExpression>
#include <QTimer>

#include "common/bcp47.h"
#include "common/iana_language_subtag_registry.h"
#include "common/iso3166.h"
#include "common/iso15924.h"
#include "common/qt.h"
#include "common/sorting.h"
#include "mkvtoolnix-gui/forms/util/language_widget.h"
#include "mkvtoolnix-gui/util/language_combo_box.h"
#include "mkvtoolnix-gui/util/language_widget.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx::gui::Util {

namespace {
template<typename Telements>
void
setupComboBoxFromList(QComboBox &comboBox,
                      Telements const &elements,
                      std::function<std::pair<QString, QString>(typename Telements::value_type const &)> extractor) {
  std::vector<std::pair<QString, QString>> items;

  for (auto const &element : elements)
    items.emplace_back(extractor(element));

  std::sort(items.begin(), items.end());

  comboBox.blockSignals(true);

  comboBox.addItem(QString{}, QString{});

  for (auto const &item : items)
    comboBox.addItem(Q("%1 (%2)").arg(item.first).arg(item.second), item.second);

  comboBox.blockSignals(false);
}

}

// ------------------------------------------------------------

class LanguageWidgetPrivate {
  friend class LanguageWidget;

  std::unique_ptr<Ui::LanguageWidget> ui{new Ui::LanguageWidget};
  QVector<QVector<QWidget *>> componentWidgets;

  QString initialISO639_2Code;
  QStringList additionalISO639_2Codes;
};

LanguageWidget::LanguageWidget(QWidget *parent)
  : QWidget{parent}
  , p_ptr{new LanguageWidgetPrivate}
{
  auto &p = *p_ptr;

  p.ui->setupUi(this);
  Util::fixScrollAreaBackground(p.ui->saComponents);

  if (Settings::get().m_bcp47LanguageEditingMode == Settings::BCP47LanguageEditingMode::FreeForm) {
    p.ui->rbFreeForm->setChecked(true);
    p.ui->leFreeForm->setFocus();

  } else {
    p.ui->rbComponentSelection->setChecked(true);
    p.ui->cbLanguage->setFocus();
  }

  createInitialComponentWidgetList();
  createGridLayoutFromComponentWidgetList();

  updateFromFreeForm();

  p.ui->cbLanguage->setup(true);
  p.ui->cbRegion->setup(true);
  setupExtendedSubtagsComboBox(*p.ui->cbExtendedSubtag1);
  setupScriptComboBox();
  setupVariantComboBox(*p.ui->cbVariant1);

  setupConnections();
  setupFreeFormAndComponentControls();
}

LanguageWidget::~LanguageWidget() {
}

void
LanguageWidget::createInitialComponentWidgetList() {
  auto &p = *p_func();

  p.componentWidgets.push_back({ p.ui->lLanguage,        p.ui->cbLanguage });
  p.componentWidgets.push_back({ p.ui->lExtendedSubtags, p.ui->cbExtendedSubtag1, p.ui->pbAddExtendedSubtag });
  p.componentWidgets.push_back({ p.ui->lScript,          p.ui->cbScript });
  p.componentWidgets.push_back({ p.ui->lRegion,          p.ui->cbRegion });
  p.componentWidgets.push_back({ p.ui->lVariants,        p.ui->cbVariant1,        p.ui->pbAddVariant });
  p.componentWidgets.push_back({ p.ui->lPrivateUse,      p.ui->lePrivateUse1,     p.ui->pbAddPrivateUse });
}

void
LanguageWidget::createGridLayoutFromComponentWidgetList() {
  auto &p = *p_func();

  delete p.ui->sawComponents->layout();

  auto layout = new QGridLayout;

  for (auto rowIdx = 0, numRows = p.componentWidgets.size(); rowIdx < numRows; ++rowIdx) {
    auto &row = p.componentWidgets[rowIdx];

    for (auto columnIdx = 0, numColumns = row.size(); columnIdx < numColumns; ++columnIdx)
      if (row[columnIdx])
        layout->addWidget(row[columnIdx], rowIdx, columnIdx, 1, 1);
  }

  p.ui->sawComponents->setLayout(layout);
  p.ui->sawComponents->updateGeometry();
}

void
LanguageWidget::retranslateUi() {
  p_func()->ui->retranslateUi(this);
}

void
LanguageWidget::reinitializeLanguageComboBox() {
  auto &p = *p_func();

  auto additionalItems = p.additionalISO639_2Codes;

  if (!p.initialISO639_2Code.isEmpty())
    additionalItems << p.initialISO639_2Code;

  auto uniqueItems = QSet<QString>{additionalItems.begin(), additionalItems.end()};

  p.ui->cbLanguage
    ->setAdditionalItems(QStringList{uniqueItems.begin(), uniqueItems.end()})
    .reInitializeIfNecessary();
}

void
LanguageWidget::setAdditionalLanguages(QStringList const &additionalLanguages) {
  auto &p = *p_func();

  p.additionalISO639_2Codes.clear();

  for (auto const &language : additionalLanguages) {
    auto parsedLanguage = mtx::bcp47::language_c::parse(to_utf8(language));

    if (parsedLanguage.is_valid() && parsedLanguage.has_valid_iso639_code())
      p.additionalISO639_2Codes << Q(parsedLanguage.get_iso639_2_code());
  }

  reinitializeLanguageComboBox();
}

void
LanguageWidget::setLanguage(QString const &language) {
  auto &p = *p_func();

  auto parsedLanguage = mtx::bcp47::language_c::parse(to_utf8(language));
  if (parsedLanguage.is_valid() && parsedLanguage.has_valid_iso639_code()) {
    p.initialISO639_2Code = Q(parsedLanguage.get_iso639_2_code());
    reinitializeLanguageComboBox();
  }

  p.ui->leFreeForm->setText(language);

  if (p.ui->rbComponentSelection->isChecked())
    updateFromFreeForm();
}

QString
LanguageWidget::language()
  const {
  return p_func()->ui->leFreeForm->text();
}

QVector<QWidget *>
LanguageWidget::allComponentWidgets() {
  auto &p = *p_func();

  QVector<QWidget *> widgets{
    p.ui->cbLanguage,
    p.ui->cbScript,
    p.ui->cbRegion,
  };

  widgets += allComponentWidgetsMatchingName(QRegularExpression{ Q("^(cb(ExtendedSubtag|Variant)|lePrivate)") });

  return widgets;
}

QVector<QWidget *>
LanguageWidget::allComponentWidgetsMatchingName(QRegularExpression const &matcher) {
  auto &p = *p_func();

  QVector<QWidget *> widgets;

  for (auto child : p.ui->sawComponents->children()) {
    auto widget = dynamic_cast<QWidget *>(child);
    if (!widget)
      continue;

    if (widget->objectName().contains(matcher))
      widgets << widget;
  }

  mtx::sort::by(widgets.begin(), widgets.end(), [](auto widget) { return mtx::sort::natural_string_c{to_utf8(widget->objectName())}; });

  return widgets;
}

void
LanguageWidget::setupConnections() {
  auto &p = *p_func();

  connect(p.ui->rbFreeForm,           &QRadioButton::clicked, this, &LanguageWidget::setupFreeFormAndComponentControls);
  connect(p.ui->rbComponentSelection, &QRadioButton::clicked, this, &LanguageWidget::setupFreeFormAndComponentControls);

  connect(p.ui->pbAddExtendedSubtag,  &QPushButton::clicked,  this, &LanguageWidget::addExtendedSubtagRowAndUpdateLayout);
  connect(p.ui->pbAddVariant,         &QPushButton::clicked,  this, &LanguageWidget::addVariantRowAndUpdateLayout);
  connect(p.ui->pbAddPrivateUse,      &QPushButton::clicked,  this, &LanguageWidget::addPrivateUseRowAndUpdateLayout);
}

void
LanguageWidget::connectComponentWidgetChange(QWidget *widget) {
  if (dynamic_cast<QComboBox *>(widget))
    connect(static_cast<QComboBox *>(widget), static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &LanguageWidget::updateFromComponents);
  else
    connect(static_cast<QLineEdit *>(widget), &QLineEdit::textChanged,                                                this, &LanguageWidget::updateFromComponents);
}

void
LanguageWidget::setupExtendedSubtagsComboBox(QComboBox &comboBox) {
  setupComboBoxFromList(comboBox, mtx::iana::language_subtag_registry::g_extlangs, [](auto const &subtag) {
    return std::make_pair(Q(subtag.description), Q(subtag.code));
  });

  comboBox.view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  Util::fixComboBoxViewWidth(comboBox);
}

void
LanguageWidget::setupScriptComboBox() {
  auto &comboBox = *p_func()->ui->cbScript;

  setupComboBoxFromList(comboBox, mtx::iso15924::g_scripts, [](auto const &script) {
    return std::make_pair(Q(script.english_name), Q(script.code));
  });

  comboBox.view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  Util::fixComboBoxViewWidth(comboBox);
}

void
LanguageWidget::setupVariantComboBox(QComboBox &comboBox) {
  setupComboBoxFromList(comboBox, mtx::iana::language_subtag_registry::g_variants, [](auto const &variant) {
    return std::make_pair(Q(variant.description), Q(variant.code));
  });

  comboBox.view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  Util::fixComboBoxViewWidth(comboBox);
}

void
LanguageWidget::setupFreeFormAndComponentControls() {
  auto &p = *p_func();

  auto componentWidgets = allComponentWidgets();

  if (p.ui->rbFreeForm->isChecked()) {
    p.ui->leFreeForm->setEnabled(true);
    Util::enableChildren(p.ui->sawComponents, false);

    for (auto widget : componentWidgets)
      if (dynamic_cast<QComboBox *>(widget))
        disconnect(static_cast<QComboBox *>(widget), static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), nullptr, nullptr);
      else
        disconnect(static_cast<QLineEdit *>(widget), &QLineEdit::textChanged,                                                nullptr, nullptr);

    connect(p.ui->leFreeForm, &QLineEdit::textChanged, this, &LanguageWidget::updateFromFreeForm);

  } else {
    p.ui->leFreeForm->setEnabled(false);
    Util::enableChildren(p.ui->sawComponents, true);

    disconnect(p.ui->leFreeForm, &QLineEdit::textChanged, nullptr, nullptr);

    for (auto widget : componentWidgets)
      connectComponentWidgetChange(widget);
  }

  maybeEnableAddExtendedSubtagButton();

  if (p.ui->rbFreeForm->isChecked())
    updateFromFreeForm();
}

void
LanguageWidget::setStatusFromLanguageTag(mtx::bcp47::language_c const &tag) {
  auto &p = *p_func();

  p.ui->lStatusOKIcon ->setVisible( tag.is_valid());
  p.ui->lStatusBadIcon->setVisible(!tag.is_valid());
  p.ui->lStatusText   ->setText(tag.is_valid() ? QY("The language tag is valid.") : QY("The language tag is not valid."));
  p.ui->lParserError  ->setText(tag.is_valid() ? QY("No error was found.")        : Q(tag.get_error()));
}

int
LanguageWidget::componentGridRowForWidget(QWidget *widget) {
  auto &p = *p_func();

  auto numRows = p.componentWidgets.size();

  for (auto row = 0; row < numRows; ++row)
    if (p.componentWidgets[row][1] == widget)
      return row;

  return -1;
}

QWidget *
LanguageWidget::addRowItem(QString const &type,
                           std::function<QWidget *(int)> const &createWidget) {
  auto &p        = *p_func();

  auto enable    = p.ui->rbComponentSelection->isChecked();
  auto items     = allComponentWidgetsMatchingName(QRegularExpression{ Q("^%1").arg(type) });
  auto newIdx    = items.size() + 1;
  auto newRow    = componentGridRowForWidget(items.last()) + 1;
  auto newWidget = createWidget(newIdx);
  auto newButton = new QPushButton{p.ui->sawComponents};

  newButton->setObjectName(Q("pbRemove%1%2").arg(type.mid(2)).arg(newIdx));
  QIcon icon;
  icon.addFile(Q(":/icons/16x16/list-remove.png"), QSize{}, QIcon::Normal, QIcon::Off);
  newButton->setIcon(icon);

  newWidget->setEnabled(enable);
  newButton->setEnabled(enable);

  if (enable)
    connectComponentWidgetChange(newWidget);
  connect(newButton, &QPushButton::clicked, this, &LanguageWidget::removeRowForClickedButton);

  p.componentWidgets.insert(newRow, { nullptr, newWidget, newButton });

  maybeEnableAddExtendedSubtagButton();

  return newWidget;
}

QWidget *
LanguageWidget::addExtendedSubtagRow() {
  return addRowItem(Q("cbExtendedSubtag"), [this](int newIdx) {
    auto comboBox = new QComboBox{p_func()->ui->sawComponents};
    comboBox->setObjectName(Q("cbExtendedSubtag%1").arg(newIdx));

    setupExtendedSubtagsComboBox(*comboBox);

    return comboBox;
  });
}

QWidget *
LanguageWidget::addVariantRow() {
  return addRowItem(Q("cbVariant"), [this](int newIdx) {
    auto comboBox = new QComboBox{p_func()->ui->sawComponents};
    comboBox->setObjectName(Q("cbVariant%1").arg(newIdx));

    setupVariantComboBox(*comboBox);

    return comboBox;
  });
}

QWidget *
LanguageWidget::addPrivateUseRow() {
  return addRowItem(Q("lePrivateUse"), [this](int newIdx) {
    auto lineEdit = new QLineEdit{p_func()->ui->sawComponents};
    lineEdit->setObjectName(Q("lePrivateUse%1").arg(newIdx));

    return lineEdit;
  });
}

void
LanguageWidget::addExtendedSubtagRowAndUpdateLayout() {
  addExtendedSubtagRow();
  createGridLayoutFromComponentWidgetList();
}

void
LanguageWidget::addVariantRowAndUpdateLayout() {
  addVariantRow();
  createGridLayoutFromComponentWidgetList();
}

void
LanguageWidget::addPrivateUseRowAndUpdateLayout() {
  addPrivateUseRow();
  createGridLayoutFromComponentWidgetList();
}

void
LanguageWidget::removeRowForClickedButton() {
  auto name = sender()->objectName();

  name.replace(QRegularExpression{Q("^pbRemove")}, Q("cb"));

  QTimer::singleShot(0, [this, name]() {
    removeRowItems(name);
    updateFromComponents();
  });
}

void
LanguageWidget::removeRowItems(QString const &namePrefix) {
  auto &p = *p_func();

  auto objectNamePattern = Q("^(cb|le|pbRemove)%1$").arg(namePrefix.mid(2));
  auto widgets           = allComponentWidgetsMatchingName(QRegularExpression{ objectNamePattern });

  for (auto widget : widgets) {
    auto row = componentGridRowForWidget(widget);
    if (row != -1)
      p.componentWidgets.removeAt(row);

    delete widget;
  }

  maybeEnableAddExtendedSubtagButton();
}

void
LanguageWidget::maybeEnableAddExtendedSubtagButton() {
  auto &p = *p_func();

  auto widgets = allComponentWidgetsMatchingName(QRegularExpression{ Q("^cbExtendedSubtag") });
  p.ui->pbAddExtendedSubtag->setEnabled(p.ui->rbComponentSelection->isChecked() && (widgets.size() < 3));
}

void
LanguageWidget::setWidgetText(QWidget &widget,
                              QString const &text) {
  if (dynamic_cast<QComboBox *>(&widget))
    setComboBoxTextByData(static_cast<QComboBox *>(&widget), text);

  else if (dynamic_cast<QLineEdit *>(&widget))
    static_cast<QLineEdit *>(&widget)->setText(text);
}

void
LanguageWidget::setMultipleWidgetsTexts(QString const &objectNamePrefix,
                                        std::vector<std::string> const &values) {
  auto widgets      = allComponentWidgetsMatchingName(QRegularExpression{ Q("^%1").arg(objectNamePrefix) });
  auto numWidgets   = widgets.size();
  auto numValues    = values.size();
  auto isVariant    = objectNamePrefix.contains(Q("Variant"));
  auto isPrivateUse = objectNamePrefix.contains(Q("PrivateUse"));

  if (!numWidgets)
    return;

  for (auto idx = 1; idx < numWidgets; ++idx)
    removeRowItems(widgets[idx]->objectName());

  setWidgetText(*widgets[0], !numValues ? QString{} : Q(values[0]));

  for (auto idx = 1u; idx < numValues; ++idx) {
    auto widget = static_cast<QWidget *>(  isVariant    ? addVariantRow()
                                         : isPrivateUse ? addPrivateUseRow()
                                         :                addExtendedSubtagRow());
    setWidgetText(*widget, Q(values[idx]));
  }

  createGridLayoutFromComponentWidgetList();
}

void
LanguageWidget::setComponentsFromLanguageTag(mtx::bcp47::language_c const &tag) {
  auto &p = *p_func();

  if (!tag.is_valid())
    return;

  setComboBoxTextByData(p.ui->cbLanguage, Q(tag.get_iso639_2_code_or({})));
  setComboBoxTextByData(p.ui->cbRegion,   Q(tag.get_region()).toLower());
  setComboBoxTextByData(p.ui->cbScript,   Q(tag.get_script()));

  setMultipleWidgetsTexts(Q("cbExtendedSubtag"), tag.get_extended_language_subtags());
  setMultipleWidgetsTexts(Q("cbVariants"),       tag.get_variants());
  setMultipleWidgetsTexts(Q("lePrivateUse"),     tag.get_private_use());
}

mtx::bcp47::language_c
LanguageWidget::languageTagFromComponents() {
  auto &p = *p_func();

  mtx::bcp47::language_c tag;
  std::vector<std::string> strings;

  tag.set_language(to_utf8(p.ui->cbLanguage->currentData().toString()));
  tag.set_script(to_utf8(p.ui->cbScript->currentData().toString()));
  tag.set_region(to_utf8(p.ui->cbRegion->currentData().toString()));

  for (auto comboBox : allComponentWidgetsMatchingName(QRegularExpression{ Q("^cbExtendedSubtag") })) {
    auto code = dynamic_cast<QComboBox &>(*comboBox).currentData().toString();
    if (!code.isEmpty())
      strings.emplace_back(to_utf8(code));
  }

  tag.set_extended_language_subtags(strings);

  strings.clear();

  for (auto comboBox : allComponentWidgetsMatchingName(QRegularExpression{ Q("^cbVariant") })) {
    auto code = dynamic_cast<QComboBox &>(*comboBox).currentData().toString();
    if (!code.isEmpty())
      strings.emplace_back(to_utf8(code));
  }

  tag.set_variants(strings);

  strings.clear();

  for (auto lineEdit : allComponentWidgetsMatchingName(QRegularExpression{ Q("^lePrivateUse") })) {
    auto text = dynamic_cast<QLineEdit &>(*lineEdit).text();
    if (!text.isEmpty())
      strings.emplace_back(to_utf8(text));
  }

  tag.set_private_use(strings);

  return tag;
}

void
LanguageWidget::updateFromFreeForm() {
  auto &p  = *p_func();
  auto tag = mtx::bcp47::language_c::parse(to_utf8(p.ui->leFreeForm->text()));

  setComponentsFromLanguageTag(tag);

  setStatusFromLanguageTag(tag);

  Q_EMIT tagValidityChanged(tag.is_valid());
}

void
LanguageWidget::updateFromComponents() {
  auto &p = *p_func();

  auto tag         = languageTagFromComponents();
  auto verifiedTag = mtx::bcp47::language_c::parse(tag.format(true));

  if (verifiedTag.is_valid())
    p.ui->leFreeForm->setText(Q(verifiedTag.format()));

  setStatusFromLanguageTag(verifiedTag);

  Q_EMIT tagValidityChanged(verifiedTag.is_valid());
}

}
