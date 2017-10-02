#pragma once

#include "common/common_pch.h"

namespace mtx { namespace gui { namespace ChapterEditor {

class TabPrivate {
  friend class Tab;

  // UI stuff:
  std::unique_ptr<Ui::Tab> ui;

  QString fileName, originalFileName;
  std::unique_ptr<QtKaxAnalyzer> analyzer;
  QDateTime fileModificationTime;

  ChapterModel *chapterModel;
  NameModel *nameModel;

  QAction *expandAllAction, *collapseAllAction, *addEditionBeforeAction, *addEditionAfterAction, *addChapterBeforeAction, *addChapterAfterAction, *addSubChapterAction, *removeElementAction;
  QAction *duplicateAction, *massModificationAction, *generateSubChaptersAction, *renumberSubChaptersAction;
  QList<QWidget *> nameWidgets;

  bool ignoreChapterSelectionChanges{};

  QString savedState;

  timestamp_c fileEndTimestamp;

  explicit TabPrivate(Tab &tab, QString const &pFileName);
};

}}}
