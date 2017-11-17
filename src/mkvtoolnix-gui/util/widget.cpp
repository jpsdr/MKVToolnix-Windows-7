#include "common/common_pch.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QIcon>
#include <QLineEdit>
#include <QList>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSettings>
#include <QSpinBox>
#include <QString>

#include "common/bitvalue.h"
#include "common/kax_analyzer.h"
#include "common/list_utils.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/util/file_dialog.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx { namespace gui { namespace Util {

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

void
enableChildren(QObject *parent,
               bool enable) {
  for (auto const &child : parent->children()) {
    auto widget = qobject_cast<QWidget *>(child);
    if (widget)
      widget->setEnabled(enable);
  }
}

QPushButton *
buttonForRole(QDialogButtonBox *box,
              QDialogButtonBox::ButtonRole role) {
  auto buttons = box->buttons();
  auto button  = boost::find_if(buttons, [box, role](auto *b) { return box->buttonRole(b) == role; });
  return button == buttons.end() ? nullptr : static_cast<QPushButton *>(*button);
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

void
fixComboBoxViewWidth(QComboBox &comboBox) {
  comboBox.setSizeAdjustPolicy(QComboBox::AdjustToContents);
  comboBox.view()->setMinimumWidth(comboBox.sizeHint().width());
}

void
addSegmentUIDFromFileToLineEdit(QWidget &parent,
                                QLineEdit &lineEdit,
                                bool append) {
  auto &settings = Util::Settings::get();
  auto dir       = settings.lastOpenDirPath();
  auto filter    = QY("Matroska and WebM files") + Q(" (*.mkv *.mka *.mks *.mk3d *.webm);;")
                 + QY("All files")               + Q(" (*)");
  auto fileName  = Util::getOpenFileName(&parent, QY("Select Matroska file to read segment UID from"), dir, filter);

  if (fileName.isEmpty())
    return;

  settings.m_lastOpenDir = QFileInfo{fileName}.path();
  settings.save();

  try {
    auto segmentUID = kax_analyzer_c::read_segment_uid_from(to_utf8(fileName));
    auto uidString  = QString{};
    auto src        = segmentUID->data();

    for (int idx = 0, numBytes = segmentUID->byte_size(); idx < numBytes; ++idx)
      uidString += Q("%1").arg(QString::number(src[idx], 16), 2, '0').toUpper();

    if (!append || lineEdit.text().isEmpty())
      lineEdit.setText(uidString);

    else
      lineEdit.setText(Q("%1,%2").arg(lineEdit.text()).arg(uidString));

  } catch (mtx::kax_analyzer_x &ex) {
    Util::MessageBox::critical(&parent)
      ->title(QY("Reading the segment UID failed"))
      .text(Q(ex.what()))
      .exec();
  }
}

}}}
