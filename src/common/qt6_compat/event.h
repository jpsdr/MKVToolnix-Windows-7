#pragma once

#include <Qt>

#include <QDropEvent>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
# include <QEnterEvent>
#else
# include <QEvent>
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)

inline Qt::MouseButtons
mtxMouseButtonsFor(QDropEvent *event) {
  return event->buttons();
}

inline Qt::KeyboardModifiers
mtxKeyboardModifiersFor(QDropEvent *event) {
  return event->modifiers();
}

using MtxQEnterEventArgType = QEnterEvent;

#else  // Qt >= 6

inline Qt::MouseButtons
mtxMouseButtonsFor(QDropEvent *event) {
  return event->mouseButtons();
}

inline Qt::KeyboardModifiers
mtxKeyboardModifiersFor(QDropEvent *event) {
  return event->keyboardModifiers();
}

using MtxQEnterEventArgType = QEvent;

#endif  // Qt >= 6
