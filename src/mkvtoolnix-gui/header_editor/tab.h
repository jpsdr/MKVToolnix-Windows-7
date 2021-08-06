#pragma once

#include "common/common_pch.h"

#include <QDateTime>

#include "mkvtoolnix-gui/header_editor/page_model.h"
#include "mkvtoolnix-gui/util/kax_analyzer.h"

class QAction;
class QMenu;

class property_element_c;

namespace mtx::gui::HeaderEditor {

namespace Ui {
class Tab;
}

using KaxAttachedPtr  = std::shared_ptr<libmatroska::KaxAttached>;

class AttachmentsPage;
class TopLevelPage;
class TrackTypePage;
class ValuePage;

class Tab : public QWidget {
public:
  enum class ModifiedConfirmationMode {
    Closing,
    Reloading,
  };

private:
  Q_OBJECT

protected:
  // UI stuff:
  std::unique_ptr<Ui::Tab> ui;

  QString m_fileName;
  std::unique_ptr<mtx::gui::Util::KaxAnalyzer> m_analyzer;

  PageModel *m_model;
  PageBase *m_segmentinfoPage{};
  AttachmentsPage *m_attachmentsPage{};
  bool m_ignoreSelectionChanges{};

  QMenu *m_treeContextMenu;
  QAction *m_expandAllAction, *m_collapseAllAction, *m_addAttachmentsAction, *m_removeAttachmentAction, *m_removeAllAttachmentsAction, *m_saveAttachmentContentAction;
  QAction *m_replaceAttachmentContentAction, *m_replaceAttachmentContentSetValuesAction;

  std::shared_ptr<EbmlElement> m_eSegmentInfo, m_eTracks;

public:
  explicit Tab(QWidget *parent, QString const &fileName);
  ~Tab();

  PageModel *model() const;

  virtual PageBase *hasBeenModified();
  virtual void retranslateUi();
  virtual void appendPage(PageBase *page, QModelIndex const &parentIdx = {});
  virtual QString const &fileName() const;
  virtual QString title() const;
  virtual void validate();
  virtual void addAttachment(KaxAttachedPtr const &attachment);
  virtual bool isClosingOrReloadingOkIfModified(ModifiedConfirmationMode mode);

Q_SIGNALS:
  void removeThisTab();

public Q_SLOTS:
  virtual void showTreeContextMenu(QPoint const &pos);
  virtual void selectionChanged(QModelIndex const &current, QModelIndex const &previous);
  virtual void load();
  virtual void save();
  virtual void expandAll();
  virtual void collapseAll();
  virtual void selectAttachmentsAndAdd();
  virtual void addAttachments(QStringList const &fileNames);
  virtual void removeAllAttachments();
  virtual void removeSelectedAttachment();
  virtual void saveAttachmentContent();
  virtual void replaceAttachmentContent(bool deriveNameAndMimeType);
  virtual void handleDroppedFiles(QStringList const &fileNames, Qt::MouseButtons mouseButtons);
  virtual void focusPage(PageBase *page);

protected:
  void setupUi();
  void setupToolTips();
  void handleSegmentInfo(kax_analyzer_data_c const &data);
  void handleTracks(kax_analyzer_data_c const &data);
  void handleAttachments();
  void populateTree();
  void resetData();
  void doModifications();
  void reportValidationFailure(bool isCritical, QModelIndex const &pageIdx);
  std::unordered_map<uint64_t, uint64_t> determineTrackUIDChanges();

  ValuePage *createValuePage(TopLevelPage &parentPage, EbmlMaster &parentMaster, property_element_c const &element);
  PageBase *currentlySelectedPage() const;

  KaxAttachedPtr createAttachmentFromFile(QString const &fileName);

  void pruneEmptyMastersForTrack(TrackTypePage &page);
  void pruneEmptyMastersForAllTracks();

public:
  static memory_cptr readFileData(QWidget *parent, QString const &fileName);
};

}
