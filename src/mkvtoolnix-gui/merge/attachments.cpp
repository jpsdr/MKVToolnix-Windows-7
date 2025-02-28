#include "common/common_pch.h"

#include <QMenu>

#include "common/bluray/disc_library.h"
#include "common/mime.h"
#include "common/qt.h"
#include "common/strings/formatting.h"
#include "mkvtoolnix-gui/merge/attachment_model.h"
#include "mkvtoolnix-gui/merge/tab.h"
#include "mkvtoolnix-gui/merge/tab_p.h"
#include "mkvtoolnix-gui/forms/merge/tab.h"
#include "mkvtoolnix-gui/util/file.h"
#include "mkvtoolnix-gui/util/file_dialog.h"
#include "mkvtoolnix-gui/util/files_drag_drop_widget.h"
#include "mkvtoolnix-gui/util/header_view_manager.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/model.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx::gui::Merge {

using namespace mtx::gui;

void
Tab::setupAttachmentsControls() {
  auto &p = *p_func();

  p.ui->attachedFiles->setModel(p.attachedFilesModel);
  p.ui->attachments->setModel(p.attachmentsModel);

  p.ui->attachedFiles->enterActivatesAllSelected(true);

  // Attached files context menu
  p.attachedFilesMenu->addAction(p.enableSelectedAttachedFilesAction);
  p.attachedFilesMenu->addAction(p.disableSelectedAttachedFilesAction);
  p.attachedFilesMenu->addSeparator();
  p.attachedFilesMenu->addAction(p.enableAllAttachedFilesAction);
  p.attachedFilesMenu->addAction(p.disableAllAttachedFilesAction);

  // Attachments context menu
  p.attachmentsMenu->addAction(p.addAttachmentsAction);
  p.attachmentsMenu->addSeparator();
  p.attachmentsMenu->addAction(p.removeAttachmentsAction);
  p.attachmentsMenu->addAction(p.removeAllAttachmentsAction);
  p.attachmentsMenu->addSeparator();
  p.attachmentsMenu->addAction(p.selectAllAttachmentsAction);

  // MIME type
  for (auto const &name : mtx::mime::sorted_type_names())
    p.ui->attachmentMIMEType->addItem(Q(name), Q(name));

  p.ui->attachmentMIMEType->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  p.ui->attachmentStyle   ->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  Util::fixComboBoxViewWidth(*p.ui->attachmentMIMEType);
  Util::fixComboBoxViewWidth(*p.ui->attachmentStyle);

  p.ui->attachmentMIMEType->lineEdit()->setClearButtonEnabled(true);

  auto &cfg = Util::Settings::get();
  cfg.handleSplitterSizes(p.ui->mergeAttachmentsSplitter);

  Util::HeaderViewManager::create(*p.ui->attachedFiles, "Merge::AttachedFiles").setDefaultSizes({ { Q("name"), 180 }, { Q("mimeType"), 180 }, { Q("size"), 100 } });

  p.enableSelectedAttachedFilesAction->setIcon(QIcon::fromTheme(Q("dialog-ok-apply")));
  p.disableSelectedAttachedFilesAction->setIcon(QIcon::fromTheme(Q("dialog-cancel")));
  p.enableAllAttachedFilesAction->setIcon(QIcon::fromTheme(Q("dialog-ok-apply")));
  p.disableAllAttachedFilesAction->setIcon(QIcon::fromTheme(Q("dialog-cancel")));

  p.addAttachmentsAction->setIcon(QIcon::fromTheme(Q("list-add")));
  p.removeAttachmentsAction->setIcon(QIcon::fromTheme(Q("list-remove")));
  p.selectAllAttachmentsAction->setIcon(QIcon::fromTheme(Q("edit-select-all")));

  // Q_SIGNALS & Q_SLOTS
  connect(p.addAttachmentsAction,               &QAction::triggered,                                                    this, &Tab::onAddAttachments);
  connect(p.removeAttachmentsAction,            &QAction::triggered,                                                    this, &Tab::onRemoveAttachments);
  connect(p.removeAllAttachmentsAction,         &QAction::triggered,                                                    this, &Tab::onRemoveAllAttachments);
  connect(p.selectAllAttachmentsAction,         &QAction::triggered,                                                    this, &Tab::onSelectAllAttachments);

  connect(p.enableAllAttachedFilesAction,       &QAction::triggered,                                                    this, [this]() { enableDisableAllAttachedFiles(true); });
  connect(p.disableAllAttachedFilesAction,      &QAction::triggered,                                                    this, [this]() { enableDisableAllAttachedFiles(false); });
  connect(p.enableSelectedAttachedFilesAction,  &QAction::triggered,                                                    this, [this]() { enableDisableSelectedAttachedFiles(true); });
  connect(p.disableSelectedAttachedFilesAction, &QAction::triggered,                                                    this, [this]() { enableDisableSelectedAttachedFiles(false); });

  connect(p.ui->attachmentDescription,          &QLineEdit::textChanged,                                                this, &Tab::onAttachmentDescriptionChanged);
  connect(p.ui->attachmentMIMEType,             &QComboBox::currentTextChanged,                                         this, &Tab::onAttachmentMIMETypeChanged);
  connect(p.ui->attachmentMIMEType,             &QComboBox::editTextChanged,                                            this, &Tab::onAttachmentMIMETypeChanged);
  connect(p.ui->attachmentName,                 &QLineEdit::textChanged,                                                this, &Tab::onAttachmentNameChanged);
  connect(p.ui->attachmentStyle,                static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &Tab::onAttachmentStyleChanged);
  connect(p.ui->attachments,                    &Util::BasicTreeView::ctrlDownPressed,                                  this, &Tab::onMoveAttachmentsDown);
  connect(p.ui->attachments,                    &Util::BasicTreeView::ctrlUpPressed,                                    this, &Tab::onMoveAttachmentsUp);
  connect(p.ui->attachments,                    &Util::BasicTreeView::customContextMenuRequested,                       this, &Tab::showAttachmentsContextMenu);
  connect(p.ui->attachments,                    &Util::BasicTreeView::deletePressed,                                    this, &Tab::onRemoveAttachments);
  connect(p.ui->attachments,                    &Util::BasicTreeView::insertPressed,                                    this, &Tab::onAddAttachments);
  connect(p.ui->attachments->selectionModel(),  &QItemSelectionModel::selectionChanged,                                 this, &Tab::onAttachmentSelectionChanged);
  connect(p.ui->attachmentsTab,                 &Util::FilesDragDropWidget::filesDropped,                               this, &Tab::addAttachments);
  connect(p.ui->moveAttachmentsDown,            &QPushButton::clicked,                                                  this, &Tab::onMoveAttachmentsDown);
  connect(p.ui->moveAttachmentsUp,              &QPushButton::clicked,                                                  this, &Tab::onMoveAttachmentsUp);
  connect(p.ui->attachedFiles,                  &Util::BasicTreeView::doubleClicked,                                    this, &Tab::toggleMuxThisForSelectedAttachedFiles);
  connect(p.ui->attachedFiles,                  &Util::BasicTreeView::allSelectedActivated,                             this, &Tab::toggleMuxThisForSelectedAttachedFiles);
  connect(p.ui->attachedFiles,                  &Util::BasicTreeView::customContextMenuRequested,                       this, &Tab::showAttachedFilesContextMenu);
  connect(p.attachedFilesModel,                 &AttachedFileModel::itemChanged,                                        this, &Tab::attachedFileItemChanged);
  connect(p.attachedFilesModel,                 &QStandardItemModel::rowsInserted,                                      this, &Tab::updateAttachmentsTabTitle);
  connect(p.attachedFilesModel,                 &QStandardItemModel::rowsRemoved,                                       this, &Tab::updateAttachmentsTabTitle);
  connect(p.attachmentsModel,                   &QStandardItemModel::rowsInserted,                                      this, &Tab::updateAttachmentsTabTitle);
  connect(p.attachmentsModel,                   &QStandardItemModel::rowsRemoved,                                       this, &Tab::updateAttachmentsTabTitle);

  onAttachmentSelectionChanged();

  Util::HeaderViewManager::create(*p.ui->attachments, "Merge::Attachments").setDefaultSizes({ { Q("name"), 180 }, { Q("mimeType"), 180 }, { Q("attachTo"), 130 }, { Q("size"), 100 } });
}

void
Tab::withSelectedAttachedFiles(std::function<void(Track &)> code) {
  auto &p = *p_func();

  if (p.currentlySettingInputControlValues)
    return;

  for (auto const &attachedFile : selectedAttachedFiles()) {
    code(*attachedFile);
    p.attachedFilesModel->attachedFileUpdated(*attachedFile);
  }
}

void
Tab::withSelectedAttachments(std::function<void(Attachment &)> code) {
  auto &p = *p_func();

  if (p.currentlySettingInputControlValues)
    return;

  for (auto const &attachment : selectedAttachments()) {
    code(*attachment);
    p.attachmentsModel->attachmentUpdated(*attachment);
  }
}

QList<Track *>
Tab::selectedAttachedFiles()
  const {
  auto &p = *p_func();

  auto attachedFiles = QList<Track *>{};
  Util::withSelectedIndexes(p.ui->attachedFiles, [&attachedFiles, &p](QModelIndex const &idx) {
    auto attachedFile = p.attachedFilesModel->attachedFileForRow(idx.row());
    if (attachedFile)
      attachedFiles << attachedFile.get();
  });

  return attachedFiles;
}

QList<Attachment *>
Tab::selectedAttachments()
  const {
  auto &p          = *p_func();
  auto attachments = QList<Attachment *>{};
  Util::withSelectedIndexes(p.ui->attachments, [&attachments, &p](QModelIndex const &idx) {
    auto attachment = p.attachmentsModel->attachmentForRow(idx.row());
    if (attachment)
      attachments << attachment.get();
  });

  return attachments;
}

void
Tab::selectAttachments(QList<Attachment *> const &attachments) {
  auto &p         = *p_func();
  auto numColumns = p.attachmentsModel->columnCount() - 1;
  auto selection  = QItemSelection{};

  for (auto const &attachment : attachments) {
    auto row = p.attachmentsModel->rowForAttachment(*attachment);
    selection.select(p.attachmentsModel->index(row, 0), p.attachmentsModel->index(row, numColumns));
  }

  p.ui->attachments->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
}

void
Tab::enableDisableSelectedAttachedFiles(bool enable) {
  withSelectedAttachedFiles([=](Track &attachedFile) {
    attachedFile.m_muxThis = enable;
  });
}

void
Tab::enableDisableAllAttachedFiles(bool enable) {
  auto &p = *p_func();

  if (p.currentlySettingInputControlValues)
    return;

  for (int row = 0, numRows = p.attachedFilesModel->rowCount(); row < numRows; ++row) {
    auto attachedFile = p.attachedFilesModel->attachedFileForRow(row);
    if (attachedFile) {
      attachedFile->m_muxThis = enable;
      p.attachedFilesModel->attachedFileUpdated(*attachedFile);
    }
  }
}

void
Tab::onAttachmentNameChanged(QString newValue) {
  withSelectedAttachments([&newValue](auto &attachment) { attachment.m_name = newValue; });
}

void
Tab::onAttachmentDescriptionChanged(QString newValue) {
  withSelectedAttachments([&newValue](auto &attachment) { attachment.m_description = newValue; });
}

void
Tab::onAttachmentMIMETypeChanged(QString newValue) {
  withSelectedAttachments([&newValue](auto &attachment) { attachment.m_MIMEType = newValue; });
}

void
Tab::onAttachmentStyleChanged(int newValue) {
  auto &p   = *p_func();
  auto data = p.ui->attachmentStyle->itemData(newValue);
  if (!data.isValid())
    return;

  auto style = data.toInt() == Attachment::ToAllFiles ? Attachment::ToAllFiles : Attachment::ToFirstFile;
  withSelectedAttachments([style](auto &attachment) { attachment.m_style = style; });
}

AttachmentPtr
Tab::prepareFileForAttaching(QString const &fileName,
                             bool alwaysAdd) {
  auto info = QFileInfo{fileName};
  if (info.size() > 0x7fffffff) {
    Util::MessageBox::critical(this)
      ->title(QY("Reading failed"))
      .text(Q("%1 %2")
            .arg(QY("The file (%1) is too big (%2).").arg(fileName).arg(Q(mtx::string::format_file_size(info.size()))))
            .arg(QY("Only files smaller than 2 GiB are supported.")))
      .exec();
    return {};
  }

  auto &cfg = Util::Settings::get();

  if (!alwaysAdd) {
    auto existingFileName = findExistingAttachmentFileName(info.fileName());
    if (existingFileName) {
      if (cfg.m_mergeAttachmentsAlwaysSkipForExistingName)
        return {};

      auto const answer = Util::MessageBox::question(this)
        ->title(QY("Attachment with same name present"))
        .text(Q("%1 %2")
              .arg(QY("An attachment with the name '%1' is already present.").arg(*existingFileName))
              .arg(QY("Do you really want to add '%1' as another attachment?").arg(QDir::toNativeSeparators(fileName))))
        .buttons(QMessageBox::Yes | QMessageBox::No | QMessageBox::NoToAll)
        .buttonLabel(QMessageBox::Yes,     QY("&Add attachment"))
        .buttonLabel(QMessageBox::No,      QY("&Skip file"))
        .buttonLabel(QMessageBox::NoToAll, QY("Always s&kip files"))
        .defaultButton(QMessageBox::No)
        .exec();

      if (answer == QMessageBox::NoToAll) {
        cfg.m_mergeAttachmentsAlwaysSkipForExistingName = true;
        cfg.save();
      }

      if (answer != QMessageBox::Yes)
        return {};
    }
  }

  auto attachment = std::make_shared<Attachment>(fileName);
  attachment->guessMIMEType();

  return attachment;
}

void
Tab::addAttachmentsFiltered(QStringList const &fileNames,
                            std::optional<std::function<bool(Attachment &)>> filter) {
  QList<AttachmentPtr> attachmentsToAdd;
  for (auto &fileName : Util::replaceDirectoriesByContainedFiles(fileNames)) {
    auto attachment = prepareFileForAttaching(fileName, false);

    if (!attachment)
      continue;

    if (filter && !(*filter)(*attachment))
      continue;

    attachmentsToAdd << attachment;
  }

  p_func()->attachmentsModel->addAttachments(attachmentsToAdd);
}

void
Tab::addAttachmentsAsCovers(QStringList const &fileNames) {
  addAttachmentsFiltered(fileNames, [](Attachment &attachment) {
    attachment.m_name = Q("cover");

    auto suffix = QFileInfo{attachment.m_fileName}.suffix();
    if (!suffix.isEmpty())
      attachment.m_name += Q(".%1").arg(suffix);

    return true;
  });
}

void
Tab::addAttachments(QStringList const &fileNames) {
  addAttachmentsFiltered(fileNames);
}

void
Tab::onAddAttachments() {
  auto fileNames = selectAttachmentsToAdd();
  if (!fileNames.isEmpty())
    addAttachments(fileNames);
}

QStringList
Tab::selectAttachmentsToAdd() {
  auto fileNames = Util::getOpenFileNames(this, QY("Add attachments"), Util::Settings::get().lastOpenDirPath(), QY("All files") + Q(" (*)"));

  if (!fileNames.isEmpty())
    Util::Settings::get().m_lastOpenDir.setPath(QFileInfo{fileNames[0]}.path());

  return fileNames;
}

void
Tab::onRemoveAttachments() {
  auto &p = *p_func();

  p.attachmentsModel->removeSelectedAttachments(p.ui->attachments->selectionModel()->selection());

  onAttachmentSelectionChanged();
}

void
Tab::onRemoveAllAttachments() {
  p_func()->attachmentsModel->reset();
  onAttachmentSelectionChanged();
}

void
Tab::onSelectAllAttachments() {
  auto &p      = *p_func();
  auto numRows = p.attachmentsModel->rowCount();
  if (!numRows)
    return;

  auto selection = QItemSelection{};
  selection.select(p.attachmentsModel->index(0, 0), p.attachmentsModel->index(numRows - 1, p.attachmentsModel->columnCount() - 1));

  p.ui->attachments->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
}

void
Tab::onAttachmentSelectionChanged() {
  auto &p        = *p_func();
  auto selection = p.ui->attachments->selectionModel()->selection();
  auto numRows   = Util::numSelectedRows(selection);

  p.ui->moveAttachmentsUp->setEnabled(!!numRows);
  p.ui->moveAttachmentsDown->setEnabled(!!numRows);

  if (!numRows) {
    setAttachmentControlValues(nullptr);
    enableAttachmentControls(false);
    return;
  }

  enableAttachmentControls(true);

  if (1 < numRows) {
    setAttachmentControlValues(nullptr);
    return;
  }

  auto idxs = selection.at(0).indexes();
  if (idxs.isEmpty() || !idxs.at(0).isValid())
    return;

  auto attachment = p.attachmentsModel->attachmentForRow(idxs.at(0).row());
  if (!attachment)
    return;

  setAttachmentControlValues(attachment.get());
}

void
Tab::enableAttachmentControls(bool enable) {
  auto &p       = *p_func();
  auto controls = std::vector<QWidget *>{p.ui->attachmentName,     p.ui->attachmentNameLabel,     p.ui->attachmentDescription, p.ui->attachmentDescriptionLabel,
                                         p.ui->attachmentMIMEType, p.ui->attachmentMIMETypeLabel, p.ui->attachmentStyle,       p.ui->attachmentStyleLabel,};
  for (auto &control : controls)
    control->setEnabled(enable);
}

void
Tab::enableAttachedFilesActions() {
  auto &p           = *p_func();
  auto numSelected  = selectedAttachedFiles().size();
  auto hasSelection = !!numSelected;
  auto hasEntries   = !!p.attachedFilesModel->rowCount();

  p.enableSelectedAttachedFilesAction->setEnabled(hasSelection);
  p.disableSelectedAttachedFilesAction->setEnabled(hasSelection);
  p.enableAllAttachedFilesAction->setEnabled(hasEntries);
  p.disableAllAttachedFilesAction->setEnabled(hasEntries);

  p.enableSelectedAttachedFilesAction->setText(QNY("&Enable selected attachment", "&Enable selected attachments", numSelected));
  p.disableSelectedAttachedFilesAction->setText(QNY("&Disable selected attachment", "&Disable selected attachments", numSelected));
}

void
Tab::enableAttachmentsActions() {
  auto &p           = *p_func();
  auto numSelected  = selectedAttachments().size();
  auto hasSelection = !!numSelected;
  auto hasEntries   = !!p.attachmentsModel->rowCount();

  p.removeAttachmentsAction->setEnabled(hasSelection);
  p.removeAllAttachmentsAction->setEnabled(hasEntries);
  p.selectAllAttachmentsAction->setEnabled(hasEntries);

  p.removeAttachmentsAction->setText(QNY("&Remove attachment", "&Remove attachments", numSelected));
}

void
Tab::setAttachmentControlValues(Attachment *attachment) {
  auto &p = *p_func();

  p.currentlySettingInputControlValues = true;

  if (!attachment && p.ui->attachmentStyle->itemData(0).isValid())
    p.ui->attachmentStyle->insertItem(0, QY("<Do not change>"));

  else if (attachment && !p.ui->attachmentStyle->itemData(0).isValid())
    p.ui->attachmentStyle->removeItem(0);

  if (!attachment) {
    p.ui->attachmentName->setText(Q(""));
    p.ui->attachmentDescription->setText(Q(""));
    p.ui->attachmentMIMEType->setEditText(Q(""));
    p.ui->attachmentStyle->setCurrentIndex(0);

  } else {
    p.ui->attachmentName->setText(        attachment->m_name);
    p.ui->attachmentDescription->setText( attachment->m_description);
    p.ui->attachmentMIMEType->setEditText(attachment->m_MIMEType);

    Util::setComboBoxIndexIf(p.ui->attachmentStyle, [&attachment](auto const &, auto const &data) { return data.isValid() && (data.toInt() == static_cast<int>(attachment->m_style)); });
  }

  p.currentlySettingInputControlValues = false;
}

void
Tab::retranslateAttachmentsUI() {
  auto &p = *p_func();

  p.attachedFilesModel->retranslateUi();
  p.attachmentsModel->retranslateUi();

  p.enableAllAttachedFilesAction->setText(QY("E&nable all attachments"));
  p.disableAllAttachedFilesAction->setText(QY("Di&sable all attachments"));

  p.addAttachmentsAction->setText(QY("&Add attachments"));
  p.removeAllAttachmentsAction->setText(QY("Remove a&ll attachments"));
  p.selectAllAttachmentsAction->setText(QY("&Select all attachments"));

  // Attachment style
  p.ui->attachmentStyle->setItemData(0, static_cast<int>(Attachment::ToAllFiles));
  p.ui->attachmentStyle->setItemData(1, static_cast<int>(Attachment::ToFirstFile));

  setupAttachmentsToolTips();
  updateAttachmentsTabTitle();
}

void
Tab::setupAttachmentsToolTips() {
  Util::setToolTip(p_func()->ui->attachments, QY("Right-click for attachment actions"));
}

void
Tab::moveAttachmentsUpOrDown(bool up) {
  auto &p          = *p_func();
  auto attachments = selectedAttachments();

  p.attachmentsModel->moveAttachmentsUpOrDown(attachments, up);

  selectAttachments(attachments);

  p.ui->attachments->scrollToFirstSelected();
}

void
Tab::onMoveAttachmentsUp() {
  moveAttachmentsUpOrDown(true);
}

void
Tab::onMoveAttachmentsDown() {
  moveAttachmentsUpOrDown(false);
}

void
Tab::showAttachmentsContextMenu(QPoint const &pos) {
  auto &p = *p_func();

  enableAttachmentsActions();
  p.attachmentsMenu->exec(p.ui->attachments->viewport()->mapToGlobal(pos));
}

void
Tab::showAttachedFilesContextMenu(QPoint const &pos) {
  auto &p = *p_func();

  enableAttachedFilesActions();
  p.attachedFilesMenu->exec(p.ui->attachedFiles->viewport()->mapToGlobal(pos));
}

void
Tab::toggleMuxThisForSelectedAttachedFiles() {
  auto allEnabled            = true;
  auto attachedFilesSelected = false;

  withSelectedAttachedFiles([&allEnabled, &attachedFilesSelected](Track const &attachedFile) {
    attachedFilesSelected = true;

    if (!attachedFile.m_muxThis)
      allEnabled = false;
  });

  if (!attachedFilesSelected)
    return;

  auto newEnabled = !allEnabled;

  withSelectedAttachedFiles([newEnabled](Track &attachedFile) { attachedFile.m_muxThis = newEnabled; });
}

void
Tab::attachedFileItemChanged(QStandardItem *item) {
  auto &p = *p_func();

  if (!item)
    return;

  auto idx = p.attachedFilesModel->indexFromItem(item);
  if (idx.column())
    return;

  auto attachedFile = p.attachedFilesModel->attachedFileForRow(idx.row());
  if (!attachedFile)
    return;

  auto newMuxThis = item->checkState() == Qt::Checked;
  if (newMuxThis == attachedFile->m_muxThis)
    return;

  attachedFile->m_muxThis = newMuxThis;
  p.attachedFilesModel->attachedFileUpdated(*attachedFile);
}

std::optional<QString>
Tab::findExistingAttachmentFileName(QString const &fileName) {
  auto &p            = *p_func();
  auto lowerFileName = fileName.toLower();

  for (int row = 0, numRows = p.attachedFilesModel->rowCount(); row < numRows; ++row) {
    auto attachedFile = p.attachedFilesModel->attachedFileForRow(row);

    if (!attachedFile || !attachedFile->m_muxThis)
      continue;

    auto existingFileName = QFileInfo{attachedFile->m_name}.fileName();
    if (lowerFileName == existingFileName.toLower())
      return existingFileName;
  }

  for (int row = 0, numRows = p.attachmentsModel->rowCount(); row < numRows; ++row) {
    auto attachment = p.attachmentsModel->attachmentForRow(row);

    if (!attachment)
      continue;

    auto existingFileName = QFileInfo{attachment->m_fileName}.fileName();
    if (lowerFileName == existingFileName.toLower())
      return existingFileName;
  }

  return {};
}

void
Tab::addAttachmentsFromIdentifiedBluray(mtx::bluray::disc_library::info_t const &info) {
  unsigned int maxSize = 0;
  boost::filesystem::path fileName;

  for (auto const &thumbnail : info.m_thumbnails) {
    auto size = thumbnail.m_width * thumbnail.m_height;
    if (size < maxSize)
      continue;

    maxSize  = size;
    fileName = thumbnail.m_file_name;
  }

  if (fileName.empty() || !boost::filesystem::is_regular_file(fileName))
    return;

  auto attachment = prepareFileForAttaching(Q(fileName.string()), true);
  if (!attachment)
    return;

  attachment->m_name = Q("cover%2").arg(Q(fileName.extension().string()).toLower());

  p_func()->attachmentsModel->addAttachments(QList<AttachmentPtr>{} << attachment);
}

void
Tab::updateAttachmentsTabTitle() {
  auto &p             = *p_func();
  auto numAttachments = p.attachedFilesModel->rowCount() + p.attachmentsModel->rowCount();

  p.ui->tabs->setTabText(2, Q("%1 (%2)").arg(QY("Atta&chments")).arg(numAttachments));
}

}
