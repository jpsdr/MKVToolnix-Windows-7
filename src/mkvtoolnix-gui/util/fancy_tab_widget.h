/**************************************************************************
 **
 ** This file is adapted from fancytabwidget.h which is part of Qt Creator
 **
 ** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
 **
 ** Contact: Nokia Corporation (info@qt.nokia.com)
 **
 **
 ** GNU General Public License Usage
 **
 ** This file may be used under the terms of the GNU General Public
 ** License version 2 as published by the Free Software Foundation and
 ** appearing in the file COPYING included in the packaging of this file.
 ** Please review the following information to ensure the GNU General
 ** Public License version 2 requirements will be met:
 ** http://www.gnu.org/licenses/old-licenses/gpl-2.0.html.
 **
 **************************************************************************/

#pragma once

#include "common/common_pch.h"

#include <memory>

#include <QtGui/QIcon>
#include <QtWidgets/QWidget>
#include <QtWidgets/QStyle>

#include <QtCore/QTimer>
#include <QtCore/QPropertyAnimation>

class QPainter;
class QStackedLayout;

namespace mtx::gui::Util {

class FancyTabBar;

class FancyTab : public QObject {
  Q_OBJECT

  Q_PROPERTY(double fader READ fader WRITE setFader)

  friend class FancyTabBar;

private:
  QPropertyAnimation m_animator;
  QWidget *m_tabbar{};
  double m_fader{};

  QIcon m_icon;
  QString m_text;
  QString m_toolTip;
  bool m_enabled{};

public:
  FancyTab(QWidget *tabbar);

  double fader();
  void setFader(double value);

  void fadeIn();
  void fadeOut();
};

class FancyTabBar : public QWidget {
  Q_OBJECT

private:
  static int const m_rounding;
  static int const m_textPadding;
  QRect m_hoverRect;
  int m_hoverIndex{-1};
  int m_currentIndex{-1};
  QVector<std::shared_ptr<FancyTab>> m_tabs;
  QTimer m_triggerTimer;
  std::unique_ptr<QStyle> m_style;

public:
  FancyTabBar(QWidget *parent = nullptr);
  virtual ~FancyTabBar();

  bool event(QEvent *event);

  void paintEvent(QPaintEvent *event);
  void paintTab(QPainter *painter, int tabIndex) const;
  void mousePressEvent(QMouseEvent *);
  void mouseMoveEvent(QMouseEvent *);
  void enterEvent(QEnterEvent *);
  void leaveEvent(QEvent *);
  bool validIndex(int index) const;

  QSize sizeHint() const;
  QSize minimumSizeHint() const;

  void setTabEnabled(int index, bool enable);
  bool isTabEnabled(int index) const;

  void insertTab(int index, QIcon const &icon, QString const &label);
  void removeTab(int index);
  void setCurrentIndex(int index);
  int currentIndex() const;

  void setTabToolTip(int index, QString toolTip);
  QString tabToolTip(int index) const;
  QIcon tabIcon(int index) const;
  QString tabText(int index) const;
  void setTabText(int index, QString const &text);
  int count() const;
  QRect tabRect(int index) const;

Q_SIGNALS:
  void currentChanged(int);

public Q_SLOTS:
  void emitCurrentIndex();

private:
  QSize tabSizeHint(bool minimum = false) const;
};

class FancyTabWidget : public QWidget {
  Q_OBJECT

private:
  FancyTabBar *m_tabBar{};
  QWidget *m_cornerWidgetContainer{};
  QStackedLayout *m_modesStack{};
  QWidget *m_selectionWidget{};

public:
  FancyTabWidget(QWidget *parent = nullptr);

  void insertTab(int index, QWidget *tab, QIcon const &icon, QString const &label);
  void appendTab(QWidget *tab, QIcon const &icon, QString const &label);
  void removeTab(int index);
  void setTabText(int index, QString const &text);
  int count() const;
  QWidget *widget(int index) const;
  void setBackgroundBrush(QBrush const &brush);
  void addCornerWidget(QWidget *widget);
  void insertCornerWidget(int pos, QWidget *widget);
  int cornerWidgetCount() const;
  void setTabToolTip(int index, QString const &toolTip);

  void paintEvent(QPaintEvent *event);

  int currentIndex() const;

  void setTabEnabled(int index, bool enable);
  bool isTabEnabled(int index) const;

  FancyTabBar *tabBar();

Q_SIGNALS:
  void currentAboutToShow(int index);
  void currentChanged(int index);

public Q_SLOTS:
  void setCurrentIndex(int index);

private Q_SLOTS:
  void showWidget(int index);
};

}
