/**************************************************************************
**
** This file is adapted from fancytabwidget.cpp which is part of Qt Creator
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

#include "common/common_pch.h"

#include "mkvtoolnix-gui/util/fancy_tab_widget.h"
#include "mkvtoolnix-gui/util/font.h"
#include "mkvtoolnix-gui/util/style_helper.h"

#include <QtCore/QDebug>

#include <QtCore/QAnimationGroup>
#include <QtCore/QPropertyAnimation>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStackedLayout>
#include <QtWidgets/QStyleFactory>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QToolTip>
#include <QtWidgets/QVBoxLayout>

namespace mtx::gui::Util {

int const FancyTabBar::m_rounding    = 22;
int const FancyTabBar::m_textPadding = 4;

FancyTab::FancyTab(QWidget *tabbar)
  : m_tabbar{tabbar}
{
  m_animator.setPropertyName("fader");
  m_animator.setTargetObject(this);
}

void
FancyTab::fadeIn() {
  m_animator.stop();
  m_animator.setDuration(80);
  m_animator.setEndValue(40);
  m_animator.start();
}

void
FancyTab::fadeOut() {
  m_animator.stop();
  m_animator.setDuration(160);
  m_animator.setEndValue(0);
  m_animator.start();
}

double
FancyTab::fader() {
  return m_fader;
}

void
FancyTab::setFader(double value) {
  m_fader = value;
  m_tabbar->update();
}

FancyTabBar::FancyTabBar(QWidget *parent)
  : QWidget{parent}
  , m_style{QStyleFactory::create("windows")}
{
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  setStyle(m_style.get());
  setMinimumWidth(qMax(2 * m_rounding, 40));
  setAttribute(Qt::WA_Hover, true);
  setFocusPolicy(Qt::NoFocus);
  setMouseTracking(true); // Needed for hover events
  m_triggerTimer.setSingleShot(true);

  // We use a zerotimer to keep the sidebar responsive
  connect(&m_triggerTimer, &QTimer::timeout, this, &FancyTabBar::emitCurrentIndex);
}

FancyTabBar::~FancyTabBar() {
}

QSize
FancyTabBar::tabSizeHint(bool minimum)
  const {
  QFont boldFont{font()};
  boldFont.setPointSizeF(StyleHelper::sidebarFontSize());
  boldFont.setBold(true);
  QFontMetrics fm{boldFont};
  int spacing       = 8;
  int width         = 60 + spacing + 2;
  int maxLabelwidth = 0;
  for (int tab=0 ; tab<count() ;++tab)
    maxLabelwidth = std::max(maxLabelwidth, Util::horizontalAdvance(fm, tabText(tab)));
  int iconHeight = minimum ? 0 : 32;
  return QSize(qMax(width, maxLabelwidth + 4), iconHeight + spacing + fm.height());
}

void
FancyTabBar::paintEvent(QPaintEvent *) {
  QPainter p{this};

  for (int i = 0; i < count(); ++i)
    if (i != currentIndex())
      paintTab(&p, i);

  // paint active tab last, since it overlaps the neighbors
  if (currentIndex() != -1)
    paintTab(&p, currentIndex());
}

// Handle hover events for mouse fade ins
void
FancyTabBar::mouseMoveEvent(QMouseEvent *e) {
  int newHover = -1;
  for (int i = 0; i < count(); ++i) {
    QRect area = tabRect(i);
    if (area.contains(e->pos())) {
      newHover = i;
      break;
    }
  }
  if (newHover == m_hoverIndex)
    return;

  if (validIndex(m_hoverIndex))
    m_tabs[m_hoverIndex]->fadeOut();

  m_hoverIndex = newHover;

  if (validIndex(m_hoverIndex)) {
    m_tabs[m_hoverIndex]->fadeIn();
    m_hoverRect = tabRect(m_hoverIndex);
  }
}

bool
FancyTabBar::event(QEvent *event) {
  if (event->type() == QEvent::ToolTip) {
    if (validIndex(m_hoverIndex)) {
      QString tt = tabToolTip(m_hoverIndex);
      if (!tt.isEmpty()) {
        QToolTip::showText(static_cast<QHelpEvent*>(event)->globalPos(), tt, this);
        return true;
      }
    }
  }
  return QWidget::event(event);
}

// Resets hover animation on mouse enter
void
FancyTabBar::enterEvent(QEnterEvent *) {
  m_hoverRect = QRect();
  m_hoverIndex = -1;
}

// Resets hover animation on mouse enter
void
FancyTabBar::leaveEvent(QEvent *) {
  m_hoverIndex = -1;
  m_hoverRect = QRect();
  for (int i = 0 ; i < m_tabs.count() ; ++i) {
    m_tabs[i]->fadeOut();
  }
}

bool
FancyTabBar::validIndex(int index)
  const {
  return (index >= 0) && (index < m_tabs.count());
}

QSize
FancyTabBar::sizeHint()
  const {
  QSize sh = tabSizeHint();
  return QSize(sh.width(), sh.height() * m_tabs.count());
}

QSize
FancyTabBar::minimumSizeHint()
  const {
  QSize sh = tabSizeHint(true);
  return QSize(sh.width(), sh.height() * m_tabs.count());
}

QRect
FancyTabBar::tabRect(int index)
  const {
  QSize sh = tabSizeHint();

  if (sh.height() * m_tabs.count() > height())
    sh.setHeight(height() / m_tabs.count());

  return QRect(0, index * sh.height(), sh.width(), sh.height());

}

// This keeps the sidebar responsive since
// we get a repaint before loading the
// mode itself
void
FancyTabBar::emitCurrentIndex() {
  Q_EMIT currentChanged(m_currentIndex);
}

void
FancyTabBar::mousePressEvent(QMouseEvent *e) {
  e->accept();
  for (int index = 0; index < m_tabs.count(); ++index) {
    if (tabRect(index).contains(e->pos())) {

      if (isTabEnabled(index)) {
        m_currentIndex = index;
        update();
        m_triggerTimer.start(0);
      }
      break;
    }
  }
}

void
FancyTabBar::paintTab(QPainter *painter,
                      int tabIndex)
  const {
  if (!validIndex(tabIndex)) {
    qWarning("invalid index");
    return;
  }
  painter->save();

  QRect rect = tabRect(tabIndex);
  bool selected = (tabIndex == m_currentIndex);
  bool enabled = isTabEnabled(tabIndex);

  if (selected)
    painter->fillRect(rect.adjusted(0, 0, 0, -1), QColor(255, 255, 255, 240));

  QString tabText(this->tabText(tabIndex));
  QRect tabTextRect(tabRect(tabIndex));
  QRect tabIconRect(tabTextRect);
  tabTextRect.translate(0, -2);
  QFont boldFont(painter->font());
  boldFont.setPointSizeF(StyleHelper::sidebarFontSize());
  boldFont.setBold(true);
  painter->setFont(boldFont);
  painter->setPen(selected ? QColor(255, 255, 255, 160) : QColor(0, 0, 0, 110));
  int textFlags = Qt::AlignCenter | Qt::AlignBottom | Qt::TextWordWrap;
  if (enabled) {
    painter->drawText(tabTextRect, textFlags, tabText);
    painter->setPen(selected ? QColor(60, 60, 60) : StyleHelper::panelTextColor());
  } else {
    painter->setPen(selected ? StyleHelper::panelTextColor() : QColor(255, 255, 255, 120));
  }
#ifndef Q_WS_MAC
  if (!selected && enabled) {
    painter->save();
    int fader = int(m_tabs[tabIndex]->fader());
    QLinearGradient grad(rect.topLeft(), rect.topRight());
    grad.setColorAt(0, Qt::transparent);
    grad.setColorAt(0.5, QColor(255, 255, 255, fader));
    grad.setColorAt(1, Qt::transparent);
    painter->fillRect(rect, grad);
    painter->setPen(QPen(grad, 1.0));
    painter->drawLine(rect.topLeft(), rect.topRight());
    painter->drawLine(rect.bottomLeft(), rect.bottomRight());
    painter->restore();
  }
#endif

  if (!enabled)
    painter->setOpacity(0.7);

  int textHeight = painter->fontMetrics().boundingRect(QRect(0, 0, width(), height()), Qt::TextWordWrap, tabText).height();
  tabIconRect.adjust(0, 4, 0, -textHeight);
  StyleHelper::drawIconWithShadow(tabIcon(tabIndex), tabIconRect, painter, enabled ? QIcon::Normal : QIcon::Disabled);

  painter->translate(0, -1);
  painter->drawText(tabTextRect, textFlags, tabText);
  painter->restore();
}

void
FancyTabBar::insertTab(int index,
                       QIcon const &icon,
                       QString const &label) {
  auto tab    = std::make_shared<FancyTab>(this);
  tab->m_icon = icon;
  tab->m_text = label;
  m_tabs.insert(index, tab);
}

void
FancyTabBar::removeTab(int index) {
  m_tabs.removeAt(index);
}

void
FancyTabBar::setCurrentIndex(int index) {
  if (isTabEnabled(index)) {
    m_currentIndex = index;
    update();
    Q_EMIT currentChanged(m_currentIndex);
  }
}

int
FancyTabBar::currentIndex()
  const {
  return m_currentIndex;
}

void
FancyTabBar::setTabToolTip(int index,
                           QString toolTip) {
  m_tabs[index]->m_toolTip = toolTip;
}

QString
FancyTabBar::tabToolTip(int index)
  const {
  return m_tabs.at(index)->m_toolTip;
}

QIcon
FancyTabBar::tabIcon(int index)
  const {
  return m_tabs.at(index)->m_icon;
}

QString
FancyTabBar::tabText(int index)
  const {
  return m_tabs.at(index)->m_text;
}

void
FancyTabBar::setTabText(int index,
                        QString const &text) {
  if ((0 > index) || (m_tabs.count() <= index))
    return;

  m_tabs.at(index)->m_text = text;
  updateGeometry();
}

int
FancyTabBar::count()
  const {
  return m_tabs.count();
}

void
FancyTabBar::setTabEnabled(int index,
                           bool enable) {
  Q_ASSERT(index < m_tabs.size());
  Q_ASSERT(index >= 0);

  if (index < m_tabs.size() && index >= 0) {
    m_tabs[index]->m_enabled = enable;
    update(tabRect(index));
  }
}

bool
FancyTabBar::isTabEnabled(int index)
  const {
  Q_ASSERT(index < m_tabs.size());
  Q_ASSERT(index >= 0);

  if (index < m_tabs.size() && index >= 0)
    return m_tabs[index]->m_enabled;

  return false;
}


//////
// FancyTabWidget
//////

FancyTabWidget::FancyTabWidget(QWidget *parent)
  : QWidget{parent}
{
  m_tabBar = new FancyTabBar(this);

  m_selectionWidget = new QWidget(this);
  QVBoxLayout *selectionLayout = new QVBoxLayout;
  selectionLayout->setSpacing(0);
  selectionLayout->setContentsMargins(0, 0, 0, 0);

  // StyledBar *bar = new StyledBar;
  // QHBoxLayout *layout = new QHBoxLayout(bar);
  // layout->setContentsMargins(0, 0, 0, 0);
  // layout->setSpacing(0);
  // layout->addWidget(new FancyColorButton(this));
  // selectionLayout->addWidget(bar);

  selectionLayout->addWidget(m_tabBar, 1);
  m_selectionWidget->setLayout(selectionLayout);
  m_selectionWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

  m_cornerWidgetContainer = new QWidget(this);
  m_cornerWidgetContainer->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
  m_cornerWidgetContainer->setAutoFillBackground(false);

  QVBoxLayout *cornerWidgetLayout = new QVBoxLayout;
  cornerWidgetLayout->setSpacing(0);
  cornerWidgetLayout->setContentsMargins(0, 0, 0, 0);
  cornerWidgetLayout->addStretch();
  m_cornerWidgetContainer->setLayout(cornerWidgetLayout);

  selectionLayout->addWidget(m_cornerWidgetContainer, 0);

  m_modesStack = new QStackedLayout;
  // m_statusBar = new QStatusBar;
  // m_statusBar->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

  QVBoxLayout *vlayout = new QVBoxLayout;
  vlayout->setContentsMargins(0, 0, 0, 0);
  vlayout->setSpacing(0);
  vlayout->addLayout(m_modesStack);
  // vlayout->addWidget(m_statusBar);

  QHBoxLayout *mainLayout = new QHBoxLayout;
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(1);
  mainLayout->addWidget(m_selectionWidget);
  mainLayout->addLayout(vlayout);
  setLayout(mainLayout);

  connect(m_tabBar, &FancyTabBar::currentChanged, this, &FancyTabWidget::showWidget);
}

void
FancyTabWidget::insertTab(int index,
                          QWidget *tab,
                          QIcon const &icon,
                          QString const &label) {
  m_modesStack->insertWidget(index, tab);
  m_tabBar->insertTab(index, icon, label);
}

void
FancyTabWidget::appendTab(QWidget *tab,
                          QIcon const &icon,
                          QString const &label) {
  auto index = count();
  m_modesStack->insertWidget(index, tab);
  m_tabBar->insertTab(index, icon, label);
}

void
FancyTabWidget::removeTab(int index) {
  m_modesStack->removeWidget(m_modesStack->widget(index));
  m_tabBar->removeTab(index);
}

void
FancyTabWidget::setTabText(int index,
                           QString const &text) {
  m_tabBar->setTabText(index, text);
  update();
}

int
FancyTabWidget::count()
  const {
  return m_modesStack->count();
}

QWidget *
FancyTabWidget::widget(int index)
  const {
  return m_modesStack->widget(index);
}

void
FancyTabWidget::setBackgroundBrush(QBrush const &brush) {
  QPalette pal = m_tabBar->palette();
  pal.setBrush(QPalette::Mid, brush);
  m_tabBar->setPalette(pal);
  pal = m_cornerWidgetContainer->palette();
  pal.setBrush(QPalette::Mid, brush);
  m_cornerWidgetContainer->setPalette(pal);
}

void
FancyTabWidget::paintEvent(QPaintEvent *) {
  if (!tabBar()->isVisible())
    return;

  QPainter painter(this);

  QRect rect = m_selectionWidget->rect().adjusted(0, 0, 1, 0);
  rect = style()->visualRect(layoutDirection(), geometry(), rect);
  StyleHelper::verticalGradient(&painter, rect, rect);
  painter.setPen(StyleHelper::borderColor());
  painter.drawLine(rect.topRight(), rect.bottomRight());

  QColor light = StyleHelper::sidebarHighlight();
  painter.setPen(light);
  painter.drawLine(rect.bottomLeft(), rect.bottomRight());
}

void
FancyTabWidget::insertCornerWidget(int pos,
                                   QWidget *widget) {
  QVBoxLayout *layout = static_cast<QVBoxLayout *>(m_cornerWidgetContainer->layout());
  layout->insertWidget(pos, widget);
}

int
FancyTabWidget::cornerWidgetCount()
  const {
  return m_cornerWidgetContainer->layout()->count();
}

void
FancyTabWidget::addCornerWidget(QWidget *widget) {
  m_cornerWidgetContainer->layout()->addWidget(widget);
}

int
FancyTabWidget::currentIndex()
  const {
  return m_tabBar->currentIndex();
}

void
FancyTabWidget::setCurrentIndex(int index) {
  if (m_tabBar->isTabEnabled(index))
    m_tabBar->setCurrentIndex(index);
}

void
FancyTabWidget::showWidget(int index) {
  Q_EMIT currentAboutToShow(index);
  m_modesStack->setCurrentIndex(index);
  Q_EMIT currentChanged(index);
}

void
FancyTabWidget::setTabToolTip(int index,
                              QString const &toolTip) {
  m_tabBar->setTabToolTip(index, toolTip);
}

void
FancyTabWidget::setTabEnabled(int index,
                              bool enable) {
  m_tabBar->setTabEnabled(index, enable);
}

bool
FancyTabWidget::isTabEnabled(int index)
  const {
  return m_tabBar->isTabEnabled(index);
}

FancyTabBar *
FancyTabWidget::tabBar() {
  return m_tabBar;
}

}
