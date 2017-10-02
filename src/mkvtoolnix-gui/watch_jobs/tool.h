#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/main_window/tool_base.h"

class QMenu;

namespace mtx { namespace gui {

namespace Jobs {
class Job;
}

namespace WatchJobs {

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
  QMenu *m_jobOutputMenu;

public:
  explicit Tool(QWidget *parent, QMenu *jobOutputMenu);
  ~Tool();

  Tab *currentJobTab();
  Tab *currentTab();

  void viewOutput(mtx::gui::Jobs::Job &job);

  int tabIndexForJobID(uint64_t id) const;

  virtual void setupUi() override;
  virtual void setupActions() override;
  virtual std::pair<QString, QString> nextPreviousWindowActionTexts() const override;

public slots:
  virtual void toolShown() override;
  virtual void closeTab(int idx);
  virtual void closeCurrentTab();
  virtual void closeAllTabs();
  virtual void saveCurrentTabOutput();
  virtual void saveAllTabs();
  virtual void enableMenuActions();
  virtual void setupTabPositions();
  virtual void retranslateUi();
  virtual void switchToCurrentJobTab();

protected:
  virtual void forEachTab(std::function<void(Tab &)> const &worker);
};

}}}
