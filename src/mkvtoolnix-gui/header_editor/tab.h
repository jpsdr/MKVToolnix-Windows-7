#ifndef MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_TAB_H
#define MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_TAB_H

#include "common/common_pch.h"

#include <QDateTime>

#include "common/qt_kax_analyzer.h"
#include "mkvtoolnix-gui/header_editor/page_model.h"

class QAction;
class QMenu;

namespace mtx { namespace gui { namespace HeaderEditor {

namespace Ui {
class Tab;
}

using KaxAttachedPtr  = std::shared_ptr<KaxAttached>;

class AttachmentsPage;

class Tab : public QWidget {
  Q_OBJECT;

protected:
  // UI stuff:
  std::unique_ptr<Ui::Tab> ui;

  QString m_fileName;
  std::unique_ptr<QtKaxAnalyzer> m_analyzer;
  QDateTime m_fileModificationTime;

  PageModel *m_model;
  PageBase *m_segmentinfoPage{};
  AttachmentsPage *m_attachmentsPage{};

  QMenu *m_treeContextMenu;
  QAction *m_expandAllAction, *m_collapseAllAction, *m_addAttachmentsAction, *m_removeAttachmentAction, *m_saveAttachmentContentAction, *m_replaceAttachmentContentAction, *m_replaceAttachmentContentSetValuesAction;

  std::shared_ptr<EbmlElement> m_eSegmentInfo, m_eTracks;

public:
  explicit Tab(QWidget *parent, QString const &fileName);
  ~Tab();

  PageModel *model() const;

  virtual bool hasBeenModified();
  virtual void retranslateUi();
  virtual void appendPage(PageBase *page, QModelIndex const &parentIdx = {});
  virtual QString const &fileName() const;
  virtual QString title() const;
  virtual void validate();
  virtual void addAttachment(KaxAttachedPtr const &attachment);

signals:
  void removeThisTab();

public slots:
  virtual void showTreeContextMenu(QPoint const &pos);
  virtual void selectionChanged(QModelIndex const &current, QModelIndex const &previous);
  virtual void load();
  virtual void save();
  virtual void expandAll();
  virtual void collapseAll();
  virtual void selectAttachmentsAndAdd();
  virtual void addAttachments(QStringList const &fileNames);
  virtual void removeSelectedAttachment();
  virtual void saveAttachmentContent();
  virtual void replaceAttachmentContent(bool deriveNameAndMimeType);
  virtual void handleDroppedFiles(QStringList const &fileNames, Qt::MouseButtons mouseButtons);

protected:
  void setupUi();
  void setupToolTips();
  void handleSegmentInfo(kax_analyzer_data_c const &data);
  void handleTracks(kax_analyzer_data_c const &data);
  void handleAttachments();
  void populateTree();
  void resetData();
  void doModifications();
  void expandCollapseAll(bool expand);
  void reportValidationFailure(bool isCritical, QModelIndex const &pageIdx);

  PageBase *currentlySelectedPage() const;

  KaxAttachedPtr createAttachmentFromFile(QString const &fileName);

public:
  static memory_cptr readFileData(QWidget *parent, QString const &fileName);
};

}}}

#endif // MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_TAB_H
