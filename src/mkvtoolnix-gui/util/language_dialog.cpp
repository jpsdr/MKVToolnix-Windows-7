#include "common/common_pch.h"

#include <QAbstractItemView>
#include <QAction>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QTimer>

#include "common/bcp47.h"
#include "common/iana_language_subtag_registry.h"
#include "common/iso639.h"
#include "common/iso3166.h"
#include "common/iso15924.h"
#include "common/qt.h"
#include "common/sorting.h"
#include "common/strings/formatting.h"
#include "mkvtoolnix-gui/forms/util/language_dialog.h"
#include "mkvtoolnix-gui/util/container.h"
#include "mkvtoolnix-gui/util/language_combo_box.h"
#include "mkvtoolnix-gui/util/language_dialog.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx::gui::Util {

char const * const s_cbExtendedSubtag = "cbExtendedSubtag";
char const * const s_cbVariant        = "cbVariant";
char const * const s_leExtension      = "leExtension";
char const * const s_lePrivateUse     = "lePrivateUse";

// ------------------------------------------------------------

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

std::vector<std::string>
partiallyFormatExtensions(mtx::bcp47::language_c const &tag) {
  std::vector<std::string> formatted;

  for (auto const &extension : tag.get_extensions())
    formatted.emplace_back(extension.format());

  return formatted;
}

}

// ------------------------------------------------------------

class LanguageDialogPrivate {
  friend class LanguageDialog;

  std::unique_ptr<Ui::LanguageDialog> ui{new Ui::LanguageDialog};
  QVector<QVector<QWidget *>> componentWidgets;

  QString initialISO639Code;
  QStringList additionalISO639Codes;
  QSet<QString> previousAllAdditionalISO639Codes;

  QMenu *replaceNormalizedMenu{};
  QAction *canonicalSectionAction{}, *replaceCanonicalAction{}, *replaceCanonicalAlwaysAction{};
  QAction *extlangSectionAction{}, *replaceExtlangAction{}, *replaceExtlangAlwaysAction{};
};

LanguageDialog::LanguageDialog(QWidget *parent)
  : QDialog{parent}
  , p_ptr{new LanguageDialogPrivate}
{
  auto &p = *p_ptr;

  p.ui->setupUi(this);

  setupReplaceNormalizedMenu();

  if (Settings::get().m_bcp47LanguageEditingMode == Settings::BCP47LanguageEditingMode::FreeForm)
    p.ui->rbFreeForm->setChecked(true);

  else
    p.ui->rbComponentSelection->setChecked(true);

  createInitialComponentWidgetList();
  createGridLayoutFromComponentWidgetList();

  updateFromFreeForm();

  p.ui->cbLanguage->setup(true);
  p.ui->cbRegion->setup(true);
  setupExtendedSubtagComboBox();
  setupScriptComboBox();
  setupVariantComboBox(*p.ui->cbVariant1);

  setupConnections();
  setupFreeFormAndComponentControls();

  retranslateUi();

  Util::restoreWidgetGeometry(this);
}

LanguageDialog::~LanguageDialog() {
}

void
LanguageDialog::saveDialogGeometry() {
  Util::saveWidgetGeometry(this);
}

void
LanguageDialog::createInitialComponentWidgetList() {
  auto &p = *p_func();

  p.componentWidgets.push_back({ p.ui->lLanguage,       p.ui->cbLanguage });
  p.componentWidgets.push_back({ p.ui->lExtendedSubtag, p.ui->cbExtendedSubtag });
  p.componentWidgets.push_back({ p.ui->lScript,         p.ui->cbScript });
  p.componentWidgets.push_back({ p.ui->lRegion,         p.ui->cbRegion });
  p.componentWidgets.push_back({ p.ui->lVariants,       p.ui->cbVariant1,    p.ui->pbAddVariant });
  p.componentWidgets.push_back({ p.ui->lExtensions,     p.ui->leExtension1,  p.ui->pbAddExtension });
  p.componentWidgets.push_back({ p.ui->lPrivateUse,     p.ui->lePrivateUse1, p.ui->pbAddPrivateUse });
}

void
LanguageDialog::createGridLayoutFromComponentWidgetList() {
  auto &p = *p_func();

  delete p.ui->sawComponents->layout();

  auto layout = new QGridLayout;
  layout->setContentsMargins(0, 0, 0, 0);

  for (int rowIdx = 0, numRows = p.componentWidgets.size(); rowIdx < numRows; ++rowIdx) {
    auto &row = p.componentWidgets[rowIdx];

    for (int columnIdx = 0, numColumns = row.size(); columnIdx < numColumns; ++columnIdx)
      if (row[columnIdx])
        layout->addWidget(row[columnIdx], rowIdx, columnIdx, 1, 1);
  }

  auto spacer = new QSpacerItem{0, 0, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding};

  layout->addItem(spacer, p.componentWidgets.size(), 1, 1, 1);

  p.ui->sawComponents->setLayout(layout);
  p.ui->sawComponents->updateGeometry();
}

void
LanguageDialog::retranslateUi() {
  auto &p = *p_func();

  p.ui->retranslateUi(this);

  p.canonicalSectionAction->setText(QY("Canonical form"));
  p.replaceCanonicalAction->setText(QY("Replace with canonical form once"));
  p.replaceCanonicalAlwaysAction->setText(QY("Always automatically replace with canonical form"));
  p.extlangSectionAction->setText(QY("Extended language subtags form"));
  p.replaceExtlangAction->setText(QY("Replace with extended language subtags form once"));
  p.replaceExtlangAlwaysAction->setText(QY("Always automatically replace with extended language subtags form"));
}

void
LanguageDialog::reinitializeLanguageComboBox() {
  auto &p = *p_func();

  auto additionalItems = p.additionalISO639Codes;
  auto tag             = mtx::bcp47::language_c::parse(to_utf8(p.ui->leFreeForm->text()), mtx::bcp47::normalization_mode_e::none);

  if (!p.initialISO639Code.isEmpty())
    additionalItems << p.initialISO639Code;

  if (tag.is_valid() && tag.has_valid_iso639_code())
    additionalItems << Q(tag.get_iso639_alpha_3_code());

  auto uniqueItems = qListToSet(additionalItems);

  if (uniqueItems == p.previousAllAdditionalISO639Codes)
    return;

  p.previousAllAdditionalISO639Codes = uniqueItems;

  p.ui->cbLanguage
    ->setAdditionalItems(qSetToList(uniqueItems))
    .reInitializeIfNecessary();
}

void
LanguageDialog::setAdditionalLanguages(QStringList const &additionalLanguages) {
  auto &p = *p_func();

  p.additionalISO639Codes.clear();

  for (auto const &language : additionalLanguages) {
    auto parsedLanguage = mtx::bcp47::language_c::parse(to_utf8(language), mtx::bcp47::normalization_mode_e::none);

    if (parsedLanguage.is_valid() && parsedLanguage.has_valid_iso639_code())
      p.additionalISO639Codes << Q(parsedLanguage.get_iso639_alpha_3_code());
  }

  reinitializeLanguageComboBox();
}

void
LanguageDialog::setLanguage(mtx::bcp47::language_c const &language) {
  auto &p = *p_func();

  if (language.is_valid() && language.has_valid_iso639_code()) {
    p.initialISO639Code = Q(language.get_iso639_alpha_3_code());
    reinitializeLanguageComboBox();
  }

  p.ui->leFreeForm->setText(Q(language.format()));

  if (p.ui->rbComponentSelection->isChecked())
    updateFromFreeForm();
}

mtx::bcp47::language_c
LanguageDialog::language()
  const {
  return mtx::bcp47::language_c::parse(to_utf8(p_func()->ui->leFreeForm->text()), mtx::bcp47::normalization_mode_e::none);
}

QVector<QWidget *>
LanguageDialog::allComponentWidgets() {
  auto &p = *p_func();

  QVector<QWidget *> widgets{
    p.ui->cbLanguage,
    p.ui->cbScript,
    p.ui->cbRegion,
  };

  auto reStr = Q("^(%1|%2|%3|%4)").arg(Q(s_cbExtendedSubtag)).arg(Q(s_cbVariant)).arg(Q(s_leExtension)).arg(Q(s_lePrivateUse));
  widgets += allComponentWidgetsMatchingName(QRegularExpression{reStr});

  return widgets;
}

QVector<QWidget *>
LanguageDialog::allComponentWidgetsMatchingName(QRegularExpression const &matcher) {
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
LanguageDialog::setupReplaceNormalizedMenu() {
  auto &p = *p_func();

  p.replaceNormalizedMenu        = new QMenu;
  p.canonicalSectionAction       = new QAction;
  p.replaceCanonicalAction       = new QAction;
  p.replaceCanonicalAlwaysAction = new QAction;
  p.extlangSectionAction         = new QAction;
  p.replaceExtlangAction         = new QAction;
  p.replaceExtlangAlwaysAction   = new QAction;

  p.canonicalSectionAction      ->setSeparator(true);
  p.extlangSectionAction        ->setSeparator(true);
  p.replaceCanonicalAlwaysAction->setCheckable(true);
  p.replaceExtlangAlwaysAction  ->setCheckable(true);

  p.replaceNormalizedMenu->addAction(p.canonicalSectionAction);
  p.replaceNormalizedMenu->addAction(p.replaceCanonicalAction);
  p.replaceNormalizedMenu->addAction(p.replaceCanonicalAlwaysAction);
  p.replaceNormalizedMenu->addAction(p.extlangSectionAction);
  p.replaceNormalizedMenu->addAction(p.replaceExtlangAction);
  p.replaceNormalizedMenu->addAction(p.replaceExtlangAlwaysAction);

  p.ui->tbReplaceNormalized->setMenu(p.replaceNormalizedMenu);

  decorateReplaceMenuEntries();
}

void
LanguageDialog::decorateReplaceMenuEntries() {
  auto &p        = *p_func();
  auto &settings = Util::Settings::get();

  p.replaceCanonicalAlwaysAction->setChecked(settings.m_bcp47NormalizationMode == mtx::bcp47::normalization_mode_e::canonical);
  p.replaceExtlangAlwaysAction  ->setChecked(settings.m_bcp47NormalizationMode == mtx::bcp47::normalization_mode_e::extlang);
}

void
LanguageDialog::changeNormalizationMode(mtx::bcp47::normalization_mode_e mode) {
  auto &settings = Util::Settings::get();
  settings.m_bcp47NormalizationMode = mode;
  settings.save();

  mtx::bcp47::language_c::set_normalization_mode(mode);

  decorateReplaceMenuEntries();
}

void
LanguageDialog::setupConnections() {
  auto &p       = *p_func();
  auto okButton = p.ui->bbOkCancel->button(QDialogButtonBox::Ok);

  connect(this,                           &LanguageDialog::tagValidityChanged, okButton, &QPushButton::setEnabled);
  connect(p.ui->bbOkCancel,               &QDialogButtonBox::accepted,         this,     &QDialog::accept);
  connect(p.ui->bbOkCancel,               &QDialogButtonBox::rejected,         this,     &QDialog::reject);
  connect(this,                           &QDialog::finished,                  this,     &LanguageDialog::saveDialogGeometry);

  connect(p.ui->rbFreeForm,               &QRadioButton::clicked,              this,     &LanguageDialog::setupFreeFormAndComponentControls);
  connect(p.ui->rbComponentSelection,     &QRadioButton::clicked,              this,     &LanguageDialog::setupFreeFormAndComponentControls);

  connect(p.ui->pbAddVariant,             &QPushButton::clicked,               this,     &LanguageDialog::addVariantRowAndUpdateLayout);
  connect(p.ui->pbAddExtension,           &QPushButton::clicked,               this,     &LanguageDialog::addExtensionRowAndUpdateLayout);
  connect(p.ui->pbAddPrivateUse,          &QPushButton::clicked,               this,     &LanguageDialog::addPrivateUseRowAndUpdateLayout);

  connect(p.replaceCanonicalAction,       &QAction::triggered,                 [this]() { replaceWithCanonicalForm(false); });
  connect(p.replaceCanonicalAlwaysAction, &QAction::triggered,                 [this]() { replaceWithCanonicalForm(true);  });
  connect(p.replaceExtlangAction,         &QAction::triggered,                 [this]() { replaceWithExtlangForm(false);   });
  connect(p.replaceExtlangAlwaysAction,   &QAction::triggered,                 [this]() { replaceWithExtlangForm(true);    });
}

void
LanguageDialog::connectComponentWidgetChange(QWidget *widget) {
  if (dynamic_cast<QComboBox *>(widget))
    connect(static_cast<QComboBox *>(widget), static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &LanguageDialog::updateFromComponents);
  else
    connect(static_cast<QLineEdit *>(widget), &QLineEdit::textChanged,                                                this, &LanguageDialog::updateFromComponents);
}

void
LanguageDialog::setupExtendedSubtagComboBox() {
  auto &comboBox = *p_func()->ui->cbExtendedSubtag;

  setupComboBoxFromList(comboBox, mtx::iana::language_subtag_registry::g_extlangs, [](auto const &subtag) {
    return std::make_pair(Q(subtag.description), Q(subtag.code));
  });

  comboBox.view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  Util::fixComboBoxViewWidth(comboBox);
}

void
LanguageDialog::setupScriptComboBox() {
  auto &comboBox = *p_func()->ui->cbScript;

  setupComboBoxFromList(comboBox, mtx::iso15924::g_scripts, [](auto const &script) {
    return std::make_pair(Q(script.english_name), Q(script.code));
  });

  comboBox.view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  Util::fixComboBoxViewWidth(comboBox);
}

void
LanguageDialog::setupVariantComboBox(QComboBox &comboBox) {
  setupComboBoxFromList(comboBox, mtx::iana::language_subtag_registry::g_variants, [](auto const &variant) {
    return std::make_pair(Q(variant.description), Q(variant.code));
  });

  comboBox.view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  Util::fixComboBoxViewWidth(comboBox);
}

void
LanguageDialog::setupFreeFormAndComponentControls() {
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

    connect(p.ui->leFreeForm, &QLineEdit::textChanged, this, &LanguageDialog::updateFromFreeForm);

    p.ui->leFreeForm->setFocus();

  } else {
    p.ui->leFreeForm->setEnabled(false);
    Util::enableChildren(p.ui->sawComponents, true);

    disconnect(p.ui->leFreeForm, &QLineEdit::textChanged, nullptr, nullptr);

    for (auto widget : componentWidgets)
      connectComponentWidgetChange(widget);

    p.ui->cbLanguage->setFocus();
  }

  if (p.ui->rbFreeForm->isChecked())
    updateFromFreeForm();
}

std::pair<QStringList, QStringList>
LanguageDialog::determineInfoAndWarningsFor(mtx::bcp47::language_c const &tag) {
  if (!tag.is_valid())
    return {};

  std::pair<QStringList, QStringList> lists;

  if (tag.has_valid_iso639_code() && !tag.has_valid_iso639_2_code() && (tag.get_closest_iso639_2_alpha_3_code() == "und"s))
    lists.second << QY("The selected language code '%1' is not an ISO 639-2 code. Players that only support the legacy Matroska language elements but not the IETF BCP 47 language elements will therefore display a different language such as 'und' (undetermined).").arg(Q(tag.get_language()));

  if (!tag.get_language().empty()) {
    auto language = mtx::iso639::look_up(tag.get_language());
    if (language && language->is_deprecated)
      lists.second << QY("The language '%1' is deprecated.").arg(Q(tag.get_language()));
  }

  auto extlang = mtx::iana::language_subtag_registry::look_up_extlang(tag.get_extended_language_subtag());
  if (extlang && extlang->is_deprecated)
    lists.second << QY("The extended language subtag '%1' is deprecated.").arg(Q(tag.get_extended_language_subtag()));

  if (!tag.get_script().empty()) {
    auto script = mtx::iso15924::look_up(tag.get_script());
    if (script && script->is_deprecated)
      lists.second << QY("The script '%1' is deprecated.").arg(Q(tag.get_script()));
  }

  if (!tag.get_region().empty()) {
    auto region = mtx::iso3166::look_up(tag.get_region());
    if (region && region->is_deprecated)
      lists.second << QY("The region '%1' is deprecated.").arg(Q(tag.get_region()));
  }

  for (auto const &variantStr : tag.get_variants()) {
    auto variant = mtx::iana::language_subtag_registry::look_up_variant(variantStr);
    if (variant && variant->is_deprecated)
      lists.second << QY("The variant '%1' is deprecated.").arg(Q(variantStr));
  }

  if (!tag.get_grandfathered().empty())
    lists.second << QY("This language tag is a grandfathered element only supported for historical reasons.");

  auto const canonical_form    = tag.clone().to_canonical_form();
  auto const extlang_form      = tag.clone().to_extlang_form();
  auto const normalizationMode = Util::Settings::get().m_bcp47NormalizationMode;

  if (   (tag               != canonical_form)
      && (tag               != extlang_form)
      && (normalizationMode == mtx::bcp47::normalization_mode_e::none))
    lists.first << QY("The corresponding canonical & extended language subtags forms are: %1.").arg(Q(canonical_form.format()));

  else {
    if (tag != canonical_form){
      auto &typeContainer = normalizationMode == mtx::bcp47::normalization_mode_e::canonical ? lists.second : lists.first;
      typeContainer << QY("The corresponding canonical form is: %1.").arg(Q(canonical_form.format()));
    }

    if (tag != extlang_form){
      auto &typeContainer = normalizationMode == mtx::bcp47::normalization_mode_e::extlang ? lists.second : lists.first;
      typeContainer << QY("The corresponding extended language subtags form is: %1.").arg(Q(extlang_form.format()));
    }
  }

  auto badVariantStr = tag.get_first_variant_not_matching_prefixes();
  if (badVariantStr.empty())
    return lists;

  auto badVariant = mtx::iana::language_subtag_registry::look_up_variant(badVariantStr);
  if (!badVariant || badVariant->prefixes.empty())
    return lists;

  QStringList sentences;
  sentences << QY("The variant '%1' is used with prefixes that aren't suited for it. Suitable prefixes are: %2.")
    .arg(Q(badVariantStr)).arg(Q(mtx::string::join(badVariant->prefixes, ", ")));

  if ((canonical_form != tag) && canonical_form.get_first_variant_not_matching_prefixes().empty())
    sentences << QY("The canonical form does use a suitable prefix.");

  if ((extlang_form != tag) && extlang_form.get_first_variant_not_matching_prefixes().empty())
    sentences << QY("The extended language subtags form does use a suitable prefix.");

  lists.second << sentences.join(Q(" "));

  return lists;
}

void
LanguageDialog::setStatusFromLanguageTag(mtx::bcp47::language_c const &tag) {
  auto &p               = *p_func();
  auto statusText       = tag.is_valid() ? QY("The language tag is valid.") : QY("The language tag is not valid.");
  auto [info, warnings] = determineInfoAndWarningsFor(tag);

  QString warningsText;

  if (warnings.isEmpty())
    warningsText = Q(u8"—");

  else {
    for (auto const &warning : warnings)
      warningsText += Q("<li>%1</li>").arg(warning.toHtmlEscaped());

    warningsText = Q("<ol style=\"margin-left:15px; -qt-list-indent: 0;\">%1</ol>").arg(warningsText);
  }

  delete p.ui->wInfoWarnings->layout();
  for (auto const &child : p.ui->wInfoWarnings->children())
    delete child;

  auto row    = 0;
  auto layout = new QGridLayout;
  layout->setContentsMargins(0, 0, 0, 0);

  QSizePolicy sizePolicy{QSizePolicy::MinimumExpanding, QSizePolicy::Preferred};

  auto addText = [&row, &layout, &p, &sizePolicy](bool isInfo, QString const &text) {
    auto iconLabel = new QLabel{p.ui->wInfoWarnings};
    auto textLabel = new QLabel{p.ui->wInfoWarnings};

    sizePolicy.setHeightForWidth(textLabel->sizePolicy().hasHeightForWidth());

    iconLabel->setPixmap({ Q(":/icons/16x16/%1.png").arg(isInfo ? Q("documentinfo") : Q("dialog-warning")) });
    iconLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    textLabel->setSizePolicy(sizePolicy);
    textLabel->setWordWrap(true);
    textLabel->setText(text);

    layout->addWidget(iconLabel, row, 0);
    layout->addWidget(textLabel, row, 1);

    ++row;
  };

  for (auto const &text : info)
    addText(true, text);

  for (auto const &text : warnings)
    addText(false, text);

  if (info.isEmpty() && warnings.isEmpty()) {
    auto textLabel = new QLabel{p.ui->wInfoWarnings};
    textLabel->setText(Q(u8"—"));

    layout->addWidget(textLabel, 0, 0);
  }

  p.ui->lStatusOKIcon ->setVisible( tag.is_valid());
  p.ui->lStatusBadIcon->setVisible(!tag.is_valid());
  p.ui->lStatusText   ->setText(statusText);
  p.ui->lParserError  ->setText(tag.is_valid() ? Q(u8"—") : Q(tag.get_error()));
  p.ui->wInfoWarnings ->setLayout(layout);
  p.ui->wInfoWarnings ->updateGeometry();
}

int
LanguageDialog::componentGridRowForWidget(QWidget *widget) {
  auto &p = *p_func();

  auto numRows = p.componentWidgets.size();

  for (auto row = 0; row < numRows; ++row)
    if (p.componentWidgets[row][1] == widget)
      return row;

  return -1;
}

QWidget *
LanguageDialog::addRowItem(QString const &type,
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
  connect(newButton, &QPushButton::clicked, this, &LanguageDialog::removeRowForClickedButton);

  p.componentWidgets.insert(newRow, { nullptr, newWidget, newButton });

  return newWidget;
}

QWidget *
LanguageDialog::addVariantRow() {
  return addRowItem(Q(s_cbVariant), [this](int newIdx) {
    auto comboBox = new QComboBox{p_func()->ui->sawComponents};
    comboBox->setObjectName(Q("%1%2").arg(Q(s_cbVariant)).arg(newIdx));

    setupVariantComboBox(*comboBox);

    return comboBox;
  });
}

QWidget *
LanguageDialog::addExtensionRow() {
  return addRowItem(Q(s_leExtension), [this](int newIdx) {
    auto lineEdit = new QLineEdit{p_func()->ui->sawComponents};
    lineEdit->setObjectName(Q("%1%2").arg(Q(s_leExtension)).arg(newIdx));

    return lineEdit;
  });
}

QWidget *
LanguageDialog::addPrivateUseRow() {
  return addRowItem(Q(s_lePrivateUse), [this](int newIdx) {
    auto lineEdit = new QLineEdit{p_func()->ui->sawComponents};
    lineEdit->setObjectName(Q("%1%2").arg(Q(s_lePrivateUse)).arg(newIdx));

    return lineEdit;
  });
}

void
LanguageDialog::addVariantRowAndUpdateLayout() {
  addVariantRow();
  createGridLayoutFromComponentWidgetList();
}

void
LanguageDialog::addExtensionRowAndUpdateLayout() {
  addExtensionRow();
  createGridLayoutFromComponentWidgetList();
}

void
LanguageDialog::addPrivateUseRowAndUpdateLayout() {
  addPrivateUseRow();
  createGridLayoutFromComponentWidgetList();
}

void
LanguageDialog::removeRowForClickedButton() {
  auto name = sender()->objectName();

  name.replace(QRegularExpression{Q("^pbRemove")}, Q("cb"));

  QTimer::singleShot(0, [this, name]() {
    removeRowItems(name);
    updateFromComponents();
  });
}

void
LanguageDialog::removeRowItems(QString const &namePrefix) {
  auto &p = *p_func();

  auto objectNamePattern = Q("^(cb|le|pbRemove)%1$").arg(namePrefix.mid(2));
  auto widgets           = allComponentWidgetsMatchingName(QRegularExpression{ objectNamePattern });

  for (auto widget : widgets) {
    auto row = componentGridRowForWidget(widget);
    if (row != -1)
      p.componentWidgets.removeAt(row);

    delete widget;
  }
}

void
LanguageDialog::enableNormalizeActions(mtx::bcp47::language_c const &currentLanguage) {
  auto &p              = *p_func();
  auto enableCanonical = currentLanguage.is_valid() && (currentLanguage != currentLanguage.clone().to_canonical_form());
  auto enableExtlang   = currentLanguage.is_valid() && (currentLanguage != currentLanguage.clone().to_extlang_form());

  p.canonicalSectionAction      ->setEnabled(enableCanonical);
  p.replaceCanonicalAction      ->setEnabled(enableCanonical);
  p.replaceCanonicalAlwaysAction->setEnabled(enableCanonical);
  p.extlangSectionAction        ->setEnabled(enableExtlang);
  p.replaceExtlangAction        ->setEnabled(enableExtlang);
  p.replaceExtlangAlwaysAction  ->setEnabled(enableExtlang);

  p.ui->tbReplaceNormalized     ->setEnabled(enableCanonical || enableExtlang);
}

void
LanguageDialog::setWidgetText(QWidget &widget,
                              QString const &text) {
  if (dynamic_cast<QComboBox *>(&widget))
    setComboBoxTextByData(static_cast<QComboBox *>(&widget), text);

  else if (dynamic_cast<QLineEdit *>(&widget))
    static_cast<QLineEdit *>(&widget)->setText(text);
}

void
LanguageDialog::setMultipleWidgetsTexts(QString const &objectNamePrefix,
                                        std::vector<std::string> const &values) {
  auto widgets      = allComponentWidgetsMatchingName(QRegularExpression{ Q("^%1").arg(objectNamePrefix) });
  auto numWidgets   = widgets.size();
  auto numValues    = values.size();
  auto isVariant    = objectNamePrefix.contains(Q("Variant"));
  auto isExtension  = objectNamePrefix.contains(Q("Extension"));
  auto isPrivateUse = objectNamePrefix.contains(Q("PrivateUse"));

  if (!numWidgets)
    return;

  for (auto idx = 1; idx < numWidgets; ++idx)
    removeRowItems(widgets[idx]->objectName());

  setWidgetText(*widgets[0], !numValues ? QString{} : Q(values[0]));

  for (auto idx = 1u; idx < numValues; ++idx) {
    auto widget = static_cast<QWidget *>(  isVariant    ? addVariantRow()
                                         : isExtension  ? addExtensionRow()
                                         : isPrivateUse ? addPrivateUseRow()
                                         :                nullptr);

    if (widget)
      setWidgetText(*widget, Q(values[idx]));
  }

  createGridLayoutFromComponentWidgetList();
}

void
LanguageDialog::setComponentsFromLanguageTag(mtx::bcp47::language_c const &tag) {
  auto &p = *p_func();

  if (!tag.is_valid())
    return;

  reinitializeLanguageComboBox();

  setComboBoxTextByData(p.ui->cbLanguage,       Q(tag.get_iso639_alpha_3_code()));
  setComboBoxTextByData(p.ui->cbExtendedSubtag, Q(tag.get_extended_language_subtag()));
  setComboBoxTextByData(p.ui->cbRegion,         Q(tag.get_region()).toLower());
  setComboBoxTextByData(p.ui->cbScript,         Q(tag.get_script()));

  setMultipleWidgetsTexts(Q(s_cbVariant),    tag.get_variants());
  setMultipleWidgetsTexts(Q(s_leExtension),  partiallyFormatExtensions(tag));
  setMultipleWidgetsTexts(Q(s_lePrivateUse), tag.get_private_use());
}

mtx::bcp47::language_c
LanguageDialog::languageTagFromComponents() {
  auto &p = *p_func();

  mtx::bcp47::language_c tag;

  tag.set_language(to_utf8(p.ui->cbLanguage->currentData().toString()));
  tag.set_extended_language_subtag(to_utf8(p.ui->cbExtendedSubtag->currentData().toString()));
  tag.set_script(to_utf8(p.ui->cbScript->currentData().toString()));
  tag.set_region(to_utf8(p.ui->cbRegion->currentData().toString()));

  std::vector<std::string> strings;

  for (auto comboBox : allComponentWidgetsMatchingName(QRegularExpression{ Q("^%1").arg(Q(s_cbVariant)) })) {
    auto code = dynamic_cast<QComboBox &>(*comboBox).currentData().toString();
    if (!code.isEmpty())
      strings.emplace_back(to_utf8(code));
  }

  tag.set_variants(strings);

  strings.clear();

  for (auto lineEdit : allComponentWidgetsMatchingName(QRegularExpression{ Q("^%1").arg(Q(s_leExtension)) })) {
    auto text = dynamic_cast<QLineEdit &>(*lineEdit).text();
    if (!text.isEmpty())
      strings.emplace_back(to_utf8(text));
  }

  if (!strings.empty()) {
    auto dummyTag = mtx::bcp47::language_c::parse(fmt::format("de-{0}", mtx::string::join(strings, "-")), mtx::bcp47::normalization_mode_e::none);
    tag.set_extensions(dummyTag.get_extensions());

  } else
    tag.set_extensions({});

  strings.clear();

  for (auto lineEdit : allComponentWidgetsMatchingName(QRegularExpression{ Q("^%1").arg(Q(s_lePrivateUse)) })) {
    auto text = dynamic_cast<QLineEdit &>(*lineEdit).text();
    if (!text.isEmpty())
      strings.emplace_back(to_utf8(text));
  }

  tag.set_private_use(strings);

  return tag;
}

void
LanguageDialog::updateFromFreeForm() {
  auto &p  = *p_func();
  auto tag = mtx::bcp47::language_c::parse(to_utf8(p.ui->leFreeForm->text()), mtx::bcp47::normalization_mode_e::none);

  setComponentsFromLanguageTag(tag);

  setStatusFromLanguageTag(tag);
  enableNormalizeActions(tag);

  Q_EMIT tagValidityChanged(tag.is_valid());
}

void
LanguageDialog::updateFromComponents() {
  auto &p = *p_func();

  auto tag         = languageTagFromComponents();
  auto verifiedTag = mtx::bcp47::language_c::parse(tag.format(true), mtx::bcp47::normalization_mode_e::none);

  if (verifiedTag.is_valid())
    p.ui->leFreeForm->setText(Q(verifiedTag.format()));

  setStatusFromLanguageTag(verifiedTag);
  enableNormalizeActions(verifiedTag);

  Q_EMIT tagValidityChanged(verifiedTag.is_valid());
}

void
LanguageDialog::replaceWithCanonicalForm(bool always) {
  auto currentLanguage = language();

  if (!currentLanguage.is_valid())
    return;

  auto canonicalForm = currentLanguage.clone().to_canonical_form();

  if (currentLanguage != canonicalForm)
    setLanguage(canonicalForm);

  if (always)
    changeNormalizationMode(mtx::bcp47::normalization_mode_e::canonical);
}

void
LanguageDialog::replaceWithExtlangForm(bool always) {
  auto currentLanguage = language();

  if (!currentLanguage.is_valid())
    return;

  auto extlangForm = currentLanguage.clone().to_extlang_form();

  if (currentLanguage != extlangForm)
    setLanguage(extlangForm);

  if (always)
    changeNormalizationMode(mtx::bcp47::normalization_mode_e::extlang);
}

int
LanguageDialog::exec() {
  auto &settings = Util::Settings::get();

  if (settings.m_bcp47NormalizationMode != mtx::bcp47::normalization_mode_e::none) {
    auto current = language();

    if (auto normalized = current.clone().normalize(settings.m_bcp47NormalizationMode); current.is_valid() && (current != normalized))
      setLanguage(normalized);
  }

  auto result = QDialog::exec();

  if (result) {
    auto current = language();

    if (auto normalized = current.clone().normalize(settings.m_bcp47NormalizationMode); current.is_valid() && (current != normalized))
      setLanguage(normalized);
  }

  return result;
}

}
