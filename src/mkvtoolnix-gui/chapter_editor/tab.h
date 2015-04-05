#ifndef MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_TAB_H
#define MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_TAB_H

#include "common/common_pch.h"

#include <QDateTime>
#include <QModelIndex>

#include "common/qt_kax_analyzer.h"

class QAction;

using ChaptersPtr = std::shared_ptr<KaxChapters>;

namespace mtx { namespace gui { namespace ChapterEditor {

namespace Ui {
class Tab;
}

class ChapterModel;

class Tab : public QWidget {
  Q_OBJECT;

protected:
  // UI stuff:
  std::unique_ptr<Ui::Tab> ui;

  QString m_fileName;
  std::unique_ptr<QtKaxAnalyzer> m_analyzer;
  QDateTime m_fileModificationTime;

  ChapterModel *m_chapterModel;

  QAction *m_expandAllAction, *m_collapseAllAction;

public:
  explicit Tab(QWidget *parent, QString const &fileName = QString{});
  ~Tab();

  ChapterModel *getChapterModel() const;

  virtual void retranslateUi();
  virtual QString const &getFileName() const;
  QString getTitle() const;

  // virtual bool hasBeenModified();
  // virtual void appendPage(PageBase *page, QModelIndex const &parentIdx = {});
  // virtual void validate();

signals:
  void removeThisTab();
  void titleChanged(QString const &title);

public slots:
  // virtual void selectionChanged(QModelIndex const &current, QModelIndex const &previous);
  virtual void newFile();
  virtual void load();
  // virtual void save();
  virtual void expandAll();
  virtual void collapseAll();

protected:
  void setupUi();
  void resetData();
  // void doModifications();
  // void reportValidationFailure(bool isCritical, QModelIndex const &pageIdx);
  void expandCollapseAll(bool expand, QModelIndex const &parentIdx = {});

  ChaptersPtr loadFromChapterFile();
  ChaptersPtr loadFromMatroskaFile();

  void resizeChapterColumnsToContents() const;
};

}}}

#endif // MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_TAB_H
