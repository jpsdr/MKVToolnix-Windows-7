#pragma once

#include <Qt>

#include <QDropEvent>
#include <QEnterEvent>

inline Qt::MouseButtons
mtxMouseButtonsFor(QDropEvent *event) {
  return event->buttons();
}

inline Qt::KeyboardModifiers
mtxKeyboardModifiersFor(QDropEvent *event) {
  return event->modifiers();
}

using MtxQEnterEventArgType = QEnterEvent;
