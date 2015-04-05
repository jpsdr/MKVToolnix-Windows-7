#ifndef MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_TAB_H
#define MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_TAB_H

#include "common/common_pch.h"

#include <QDateTime>
#include <QModelIndex>

#include "common/qt_kax_analyzer.h"
#include "mkvtoolnix-gui/chapter_editor/chapter_model.h"

class QAction;
class QItemSelection;

using ChaptersPtr = std::shared_ptr<KaxChapters>;

namespace libebml {
class EbmlBinary;
};

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
  QList<QWidget *> m_nameWidgets;

public:
  explicit Tab(QWidget *parent, QString const &fileName = QString{});
  ~Tab();

  ChapterModel *getChapterModel() const;

  virtual void retranslateUi();
  virtual QString const &getFileName() const;
  QString getTitle() const;

signals:
  void removeThisTab();
  void titleChanged(QString const &title);

public slots:
  virtual void newFile();
  virtual void load();
  // virtual void save();
  virtual void expandAll();
  virtual void collapseAll();

  virtual void chapterSelectionChanged(QItemSelection const &selected, QItemSelection const &deselected);

protected:
  void setupUi();
  void resetData();
  void expandCollapseAll(bool expand, QModelIndex const &parentIdx = {});

  ChaptersPtr loadFromChapterFile();
  ChaptersPtr loadFromMatroskaFile();

  void resizeChapterColumnsToContents() const;

  void copyControlsToStorage(QModelIndex const &idx);
  bool setControlsFromStorage(QModelIndex const &idx);
  bool setChapterControlsFromStorage(ChapterPtr const &chapter);
  bool setEditionControlsFromStorage(EditionPtr const &edition);

protected:
  static QString formatEbmlBinary(EbmlBinary *binary);
};

}}}

#endif // MTX_MKVTOOLNIX_GUI_CHAPTER_EDITOR_TAB_H
