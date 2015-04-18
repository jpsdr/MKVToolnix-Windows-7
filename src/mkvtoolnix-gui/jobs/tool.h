#ifndef MTX_MKVTOOLNIX_GUI_JOBS_JOBS_WIDGET_H
#define MTX_MKVTOOLNIX_GUI_JOBS_JOBS_WIDGET_H

#include "common/common_pch.h"

#include <QList>

#include "mkvtoolnix-gui/main_window/tool_base.h"
#include "mkvtoolnix-gui/jobs/model.h"

class QAction;

namespace mtx { namespace gui { namespace Jobs {

namespace Ui {
class Tool;
}

class Tool : public ToolBase {
  Q_OBJECT;

protected:
  // UI stuff:
  std::unique_ptr<Ui::Tool> ui;

  Model *m_model;

  QAction *m_startAction, *m_removeAction, *m_removeDoneAction, *m_removeDoneOkAction, *m_removeAllAction;

public:
  explicit Tool(QWidget *parent = nullptr);
  ~Tool();

  Model *model() const;
  void addJob(JobPtr const &job);

  virtual void retranslateUi() override;

public slots:
  void onStart();
  void onRemove();
  void onRemoveDone();
  void onRemoveDoneOk();
  void onRemoveAll();

  void onContextMenu(QPoint pos);

  void resizeColumnsToContents() const;

  virtual void toolShown() override;

protected:
  void setupUiControls();
};

}}}

#endif // MTX_MKVTOOLNIX_GUI_JOBS_JOB_WIDGET_H
