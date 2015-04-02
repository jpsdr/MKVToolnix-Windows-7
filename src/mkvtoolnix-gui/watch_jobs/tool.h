#ifndef MTX_MKVTOOLNIX_GUI_WATCH_JOBS_TOOL_H
#define MTX_MKVTOOLNIX_GUI_WATCH_JOBS_TOOL_H

#include "common/common_pch.h"

#include "mkvtoolnix-gui/main_window/tool_base.h"

namespace mtx { namespace gui { namespace WatchJobs {

namespace Ui {
class Tool;
}

class Tab;

class Tool : public ToolBase {
  Q_OBJECT;

protected:
  // UI stuff:
  std::unique_ptr<Ui::Tool> ui;
  Tab *m_currentJobTab{};

public:
  explicit Tool(QWidget *parent = nullptr);
  ~Tool();

  Tab *currentJobTab();

  virtual void retranslateUi() override;

public slots:
  virtual void toolShown() override;
};

}}}

#endif // MTX_MKVTOOLNIX_GUI_WATCH_JOBS_TOOL_H
