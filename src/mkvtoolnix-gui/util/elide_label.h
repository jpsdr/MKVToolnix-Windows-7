#pragma once

#include "common/common_pch.h"

#include <Qt>
#include <QFrame>

class QString;
class QEvent;
class QPaintEvent;

namespace mtx { namespace gui { namespace Util {

class ElideLabel: public QFrame {
  Q_OBJECT;
  Q_PROPERTY(Qt::TextElideMode elideMode READ elideMode WRITE setElideMode);

protected:
  QString m_text;
  Qt::TextElideMode m_elideMode;

public:
  explicit ElideLabel(QWidget *parent = nullptr, Qt::WindowFlags flags = 0);
  explicit ElideLabel(QString const &text, QWidget *parent = nullptr, Qt::WindowFlags flags = 0);
  virtual ~ElideLabel();

  QString text() const;

  Qt::TextElideMode elideMode() const;
  void setElideMode(Qt::TextElideMode mode);

  virtual QSize sizeHint() const;
  virtual QSize minimumSizeHint() const;

public slots:
  void setText(QString const &text);

signals:
  void textChanged(QString const &text);

protected:
  virtual void changeEvent(QEvent *event);
  virtual void paintEvent(QPaintEvent *event);

  void initVars();
  void updateLabel();
};

}}}
