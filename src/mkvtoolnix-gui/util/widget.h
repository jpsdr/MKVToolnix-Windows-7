#pragma once

#include "common/common_pch.h"

#include <QDialogButtonBox>

class QComboBox;
class QGroupBox;
class QIcon;
class QLineEdit;
class QPushButton;
class QScrollArea;
class QString;
class QTabWidget;
class QVariant;
class QWidget;

namespace mtx::gui::Util {

void setToolTip(QWidget *widget, QString const &toolTip);

bool setComboBoxIndexIf(QComboBox *comboBox, std::function<bool(QString const &, QVariant const &)> test);
bool setComboBoxTextByData(QComboBox *comboBox, QString const &data);
void setComboBoxTexts(QComboBox *comboBox, QStringList const &texts);
void fixComboBoxViewWidth(QComboBox &comboBox);

void enableWidgets(QList<QWidget *> const &widgets, bool enable);
QPushButton *buttonForRole(QDialogButtonBox *box, QDialogButtonBox::ButtonRole role = QDialogButtonBox::AcceptRole);

void saveWidgetGeometry(QWidget *widget);
bool restoreWidgetGeometry(QWidget *widget);

QWidget *tabWidgetCloseTabButton(QTabWidget &tabWidget, int tabIdx);
void fixScrollAreaBackground(QScrollArea *scrollArea);
void preventScrollingWithoutFocus(QObject *parent);

void enableChildren(QObject *parent, bool enable);

void addSegmentUIDFromFileToLineEdit(QWidget &parent, QLineEdit &lineEdit, bool append);

void setupTabWidgetHeaders(QTabWidget &tabWidget);

void autoGroupBoxGridLayout(QGroupBox &box, unsigned int numColumns);

}
