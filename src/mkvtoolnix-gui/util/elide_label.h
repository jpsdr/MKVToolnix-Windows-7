#pragma once

#include "common/common_pch.h"

#include <Qt>
#include <QFrame>

#include "common/qt.h"

class QString;
class QEvent;
class QPaintEvent;

namespace mtx::gui::Util {

class ElideLabelPrivate;
class ElideLabel: public QFrame {
  Q_OBJECT

protected:
  MTX_DECLARE_PRIVATE(ElideLabelPrivate)

  std::unique_ptr<ElideLabelPrivate> const p_ptr;

  explicit ElideLabel(QWidget *parent, ElideLabelPrivate &p);

public:
  explicit ElideLabel(QWidget *parent = nullptr, Qt::WindowFlags flags = 0);
  explicit ElideLabel(QString const &text, QWidget *parent = nullptr, Qt::WindowFlags flags = 0);
  virtual ~ElideLabel();

  QString text() const;

  Qt::TextElideMode elideMode() const;
  void setElideMode(Qt::TextElideMode mode);

  virtual QSize sizeHint() const;
  virtual QSize minimumSizeHint() const;

public Q_SLOTS:
  void setText(QString const &text);

Q_SIGNALS:
  void textChanged(QString const &text);

protected:
  virtual void changeEvent(QEvent *event);
  virtual void paintEvent(QPaintEvent *event);

  void updateLabel();
};

}
