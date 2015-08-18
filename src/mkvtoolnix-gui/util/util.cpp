#include "common/common_pch.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QIcon>
#include <QList>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSpinBox>
#include <QSettings>
#include <QString>
#include <QTableView>
#include <QTreeView>

#include "common/list_utils.h"
#include "common/qt.h"
#include "common/strings/editing.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/util.h"

namespace mtx { namespace gui { namespace Util {

std::vector<std::string>
toStdStringVector(QStringList const &strings,
                  int offset) {
  auto stdStrings = std::vector<std::string>{};
  auto numStrings = strings.count();

  if (offset > numStrings)
    return stdStrings;

  stdStrings.reserve(numStrings - offset);

  for (auto idx = offset; idx < numStrings; ++idx)
    stdStrings.emplace_back(to_utf8(strings[idx]));

  return stdStrings;
}

QStringList
toStringList(std::vector<std::string> const &stdStrings,
             int offset) {
  auto strings       = QStringList{};
  auto numStdStrings = static_cast<int>(stdStrings.size());

  if (offset > numStdStrings)
    return strings;

  strings.reserve(numStdStrings - offset);

  for (auto idx = offset; idx < numStdStrings; ++idx)
    strings << to_qs(stdStrings[idx]);

  return strings;
}

QIcon
loadIcon(QString const &name,
         QList<int> const &sizes) {
  QIcon icon;
  for (auto size : sizes)
    icon.addFile(QString{":/icons/%1x%1/%2"}.arg(size).arg(name));

  return icon;
}

bool
setComboBoxIndexIf(QComboBox *comboBox,
                   std::function<bool(QString const &, QVariant const &)> test) {
  auto count = comboBox->count();
  for (int idx = 0; count > idx; ++idx)
    if (test(comboBox->itemText(idx), comboBox->itemData(idx))) {
      comboBox->setCurrentIndex(idx);
      return true;
    }

  return false;
}

bool
setComboBoxTextByData(QComboBox *comboBox,
                      QString const &data) {
  return setComboBoxIndexIf(comboBox, [&data](QString const &, QVariant const &itemData) { return itemData.isValid() && (itemData.toString() == data); });
}

void
setComboBoxTexts(QComboBox *comboBox,
                 QStringList const &texts) {
  auto numItems    = comboBox->count();
  auto numTexts    = texts.count();
  auto textIdx     = 0;
  auto comboBoxIdx = 0;

  while ((comboBoxIdx < numItems) && (textIdx < numTexts)) {
    if (comboBox->itemData(comboBoxIdx).isValid()) {
      comboBox->setItemText(comboBoxIdx, texts[textIdx]);
      ++textIdx;
    }

    ++comboBoxIdx;
  }
}

void
enableWidgets(QList<QWidget *> const &widgets,
              bool enable) {
  for (auto &widget : widgets)
    widget->setEnabled(enable);
}

QPushButton *
buttonForRole(QDialogButtonBox *box,
              QDialogButtonBox::ButtonRole role) {
  auto buttons = box->buttons();
  auto button  = boost::find_if(buttons, [&](QAbstractButton *b) { return box->buttonRole(b) == role; });
  return button == buttons.end() ? nullptr : static_cast<QPushButton *>(*button);
}

void
resizeViewColumnsToContents(QTableView *view) {
  auto columnCount = view->model()->columnCount(QModelIndex{});
  for (auto column = 0; columnCount > column; ++column)
    view->resizeColumnToContents(column);
}

void
resizeViewColumnsToContents(QTreeView *view) {
  auto columnCount = view->model()->columnCount(QModelIndex{});
  for (auto column = 0; columnCount > column; ++column)
    view->resizeColumnToContents(column);
}

void
withSelectedIndexes(QAbstractItemView *view,
                    std::function<void(QModelIndex const &)> worker) {
  withSelectedIndexes(view->selectionModel(), worker);
}

void
withSelectedIndexes(QItemSelectionModel *selectionModel,
                    std::function<void(QModelIndex const &)> worker) {
  auto rowsSeen = QMap< std::pair<QModelIndex, int>, bool >{};
  for (auto const &range : selectionModel->selection())
    for (auto const &index : range.indexes()) {
      auto seenIdx = std::make_pair(index.parent(), index.row());
      if (rowsSeen[seenIdx])
        continue;
      rowsSeen[seenIdx] = true;
      worker(index.sibling(index.row(), 0));
    }
}

int
numSelectedRows(QItemSelection &selection) {
  auto rowsSeen = QMap< std::pair<QModelIndex, int>, bool >{};
  for (auto const &range : selection)
    for (auto const &index : range.indexes()) {
      auto seenIdx      = std::make_pair(index.parent(), index.row());
      rowsSeen[seenIdx] = true;
    }

  return rowsSeen.count();
}

QModelIndex
selectedRowIdx(QItemSelection const &selection) {
  if (selection.isEmpty())
    return {};

  auto indexes = selection.at(0).indexes();
  if (indexes.isEmpty() || !indexes.at(0).isValid())
    return {};

  auto idx = indexes.at(0);
  return idx.sibling(idx.row(), 0);
}

QModelIndex
selectedRowIdx(QAbstractItemView *view) {
  if (!view)
    return {};
  return selectedRowIdx(view->selectionModel()->selection());
}

void
selectRow(QAbstractItemView *view,
          int row,
          QModelIndex const &parentIdx) {
  auto itemModel      = view->model();
  auto selectionModel = view->selectionModel();
  auto selection      = QItemSelection{itemModel->index(row, 0, parentIdx), itemModel->index(row, itemModel->columnCount() - 1, parentIdx)};
  selectionModel->select(selection, QItemSelectionModel::ClearAndSelect);
}

QModelIndex
toTopLevelIdx(QModelIndex const &idx) {
  if (!idx.isValid())
    return QModelIndex{};

  auto parent = idx.parent();
  return parent == QModelIndex{} ? idx : parent;
}

static QString
escapeMkvtoolnix(QString const &source) {
  if (source.isEmpty())
    return QString{"#EMPTY#"};
  return to_qs(::escape(to_utf8(source)));
}

static QString
unescapeMkvtoolnix(QString const &source) {
  if (source == Q("#EMPTY#"))
    return Q("");
  return to_qs(::unescape(to_utf8(source)));
}

static QString
escapeShellUnix(QString const &source) {
  if (source.isEmpty())
    return Q("\"\"");

  if (!source.contains(QRegExp{"[^\\w%+,\\-./:=@]"}))
    return source;

  auto copy = source;
  // ' -> '\''
  copy.replace(QRegExp{"'"}, Q("'\\''"));

  copy = Q("'%1'").arg(copy);
  copy.replace(QRegExp{"^''"}, Q(""));
  copy.replace(QRegExp{"''$"}, Q(""));

  return copy;
}

static QString
escapeShellWindows(QString const &source) {
  if (source.isEmpty())
    return Q("^\"^\"");

  if (!source.contains(QRegExp{"[\\w+,\\-./:=@]"}))
    return source;

  auto copy = QString{'"'};

  for (auto it = source.begin(), end = source.end() ; ; ++it) {
    QString::size_type numBackslashes = 0;

    while ((it != end) && (*it == QChar{'\\'})) {
      ++it;
      ++numBackslashes;
    }

    if (it == end) {
      copy += QString{numBackslashes * 2, QChar{'\\'}};
      break;

    } else if (*it == QChar{'"'})
      copy += QString{numBackslashes * 2 + 1, QChar{'\\'}} + *it;

    else
      copy += QString{numBackslashes, QChar{'\\'}} + *it;
  }

  copy += QChar{'"'};

  copy.replace(QRegExp{"([()%!^\"<>&|])"}, Q("^\\1"));

  return copy;
}

static QString
escapeShellWindowsProgram(QString const &source) {
  if (source.contains(QRegularExpression{"[&<>[\\]{}^=;!'+,`~ ]"}))
    return Q("\"%1\"").arg(source);

  return source;
}

QString
escape(QString const &source,
       EscapeMode mode) {
  return EscapeMkvtoolnix          == mode ? escapeMkvtoolnix(source)
       : EscapeShellUnix           == mode ? escapeShellUnix(source)
       : EscapeShellCmdExeArgument == mode ? escapeShellWindows(source)
       : EscapeShellCmdExeProgram  == mode ? escapeShellWindowsProgram(source)
       :                                     source;
}

QString
unescape(QString const &source,
         EscapeMode mode) {
  Q_ASSERT(EscapeMkvtoolnix == mode);

  return unescapeMkvtoolnix(source);
}

QStringList
escape(QStringList const &source,
       EscapeMode mode) {
  auto escaped = QStringList{};
  auto first   = true;

  for (auto const &string : source) {
    escaped << escape(string, first && (EscapeShellCmdExeArgument == mode) ? EscapeShellCmdExeProgram : mode);
    first = false;
  }

  return escaped;
}

QStringList
unescape(QStringList const &source,
         EscapeMode mode) {
  auto unescaped = QStringList{};
  for (auto const &string : source)
    unescaped << unescape(string, mode);

  return unescaped;
}

QString
joinSentences(QStringList const &sentences) {
  // TODO: act differently depending on the UI locale. Some languages,
  // e.g. Japanese, don't join sentences with spaces.
  return sentences.join(" ");
}

QString
displayableDate(QDateTime const &date) {
  return date.isValid() ? date.toString(QString{"yyyy-MM-dd hh:mm:ss"}) : QString{""};
}

QString
itemFlagsToString(Qt::ItemFlags const &flags) {
  auto items = QStringList{};

  if (flags & Qt::ItemIsSelectable)     items << "IsSelectable";
  if (flags & Qt::ItemIsEditable)       items << "IsEditable";
  if (flags & Qt::ItemIsDragEnabled)    items << "IsDragEnabled";
  if (flags & Qt::ItemIsDropEnabled)    items << "IsDropEnabled";
  if (flags & Qt::ItemIsUserCheckable)  items << "IsUserCheckable";
  if (flags & Qt::ItemIsEnabled)        items << "IsEnabled";
  if (flags & Qt::ItemIsTristate)       items << "IsTristate";
  if (flags & Qt::ItemNeverHasChildren) items << "NeverHasChildren";

  return items.join(Q("|"));
}

void
setToolTip(QWidget *widget,
           QString const &toolTip) {
  // Qt up to and including 5.3 only word-wraps tool tips
  // automatically if the format is recognized to be Rich Text. See
  // http://doc.qt.io/qt-5/qstandarditem.html

  widget->setToolTip(toolTip.startsWith('<') ? toolTip : Q("<span>%1</span>").arg(toolTip.toHtmlEscaped()));
}

void
saveWidgetGeometry(QWidget *widget) {
  auto reg = Util::Settings::registry();

  reg->beginGroup("windowGeometry");
  reg->setValue(widget->objectName(), widget->saveGeometry());
  reg->endGroup();
}

void
restoreWidgetGeometry(QWidget *widget) {
  auto reg = Util::Settings::registry();

  reg->beginGroup("windowGeometry");
  widget->restoreGeometry(reg->value(widget->objectName()).toByteArray());
  reg->endGroup();
}

QWidget *
tabWidgetCloseTabButton(QTabWidget &tabWidget,
                        int tabIdx) {
  auto tabBar = tabWidget.tabBar();
  auto result = mtx::first_of<QWidget *>([](QWidget *button) { return !!button; }, tabBar->tabButton(tabIdx, QTabBar::LeftSide), tabBar->tabButton(tabIdx, QTabBar::RightSide));
  return result ? result.get() : nullptr;
}

void
fixScrollAreaBackground(QScrollArea *scrollArea) {
  scrollArea->setBackgroundRole(QPalette::Base);
}

void
preventScrollingWithoutFocus(QObject *parent) {
  auto install = [](QWidget *widget) {
    widget->installEventFilter(MainWindow::get());
    widget->setFocusPolicy(Qt::StrongFocus);
  };

  for (auto const &child : parent->findChildren<QCheckBox *>())
    install(child);

  for (auto const &child : parent->findChildren<QComboBox *>())
    install(child);

  for (auto const &child : parent->findChildren<QRadioButton *>())
    install(child);

  for (auto const &child : parent->findChildren<QSpinBox *>())
    install(child);
}

}}}
