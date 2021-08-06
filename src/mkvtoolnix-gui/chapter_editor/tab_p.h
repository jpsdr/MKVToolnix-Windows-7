#pragma once

#include "common/common_pch.h"

namespace mtx::gui::ChapterEditor {

class TabPrivate {
  friend class Tab;

  // UI stuff:
  std::unique_ptr<Ui::Tab> ui;

  QString fileName, originalFileName;
  std::unique_ptr<mtx::gui::Util::KaxAnalyzer> analyzer;
  QDateTime fileModificationTime;

  ChapterModel *chapterModel;
  NameModel *nameModel;

  QAction *expandAllAction, *collapseAllAction, *addEditionBeforeAction, *addEditionAfterAction, *addChapterBeforeAction, *addChapterAfterAction, *addSubChapterAction, *removeElementAction;
  QAction *duplicateAction, *massModificationAction, *generateSubChaptersAction, *renumberSubChaptersAction;
  QMenu *copyToOtherTabMenu;
  QList<QWidget *> nameWidgets;

  bool ignoreChapterSelectionChanges{}, ignoreChapterNameChanges{};

  QString savedState;

  timestamp_c fileEndTimestamp;

  QVector<std::tuple<QBoxLayout *, QWidget *, QPushButton *>> languageControls, countryControls;

  explicit TabPrivate(Tab &tab, QString const &pFileName);
};

}
