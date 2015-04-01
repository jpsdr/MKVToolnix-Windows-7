#ifndef MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_TOOL_BASE_H
#define MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_TOOL_BASE_H

#include "common/common_pch.h"

#include <QWidget>

class ToolBase : public QWidget {
  Q_OBJECT;

public:
  explicit ToolBase(QWidget *parent) : QWidget{parent} {}
  virtual ~ToolBase() {};

  virtual void retranslateUi() = 0;

public slots:
  virtual void toolShown() = 0;
};

#endif // MTX_MKVTOOLNIX_GUI_MAIN_WINDOW_TOOL_BASE_H
