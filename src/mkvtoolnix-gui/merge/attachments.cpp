#include "common/common_pch.h"

#include <QFileDialog>
#include <QMenu>

#include "common/extern_data.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/merge/tab.h"
#include "mkvtoolnix-gui/forms/merge/tab.h"
#include "mkvtoolnix-gui/util/files_drag_drop_widget.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/util.h"

namespace mtx { namespace gui { namespace Merge {

using namespace mtx::gui;

void
Tab::setupAttachmentsControls() {
  ui->attachments->dropInFirstColumnOnly(false);
  ui->attachments->setModel(m_attachmentsModel);
  ui->splitMaxFiles->setMaximum(std::numeric_limits<int>::max());

  // MIME type
  for (auto &mime_type : mime_types)
    ui->attachmentMIMEType->addItem(to_qs(mime_type.name), to_qs(mime_type.name));

  // Context menu
  m_attachmentsMenu->addAction(m_addAttachmentsAction);
  m_attachmentsMenu->addAction(m_removeAttachmentsAction);

  ui->attachmentMIMEType->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  ui->attachmentStyle   ->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  // Signals & slots
  connect(ui->attachmentsTab,                &Util::FilesDragDropWidget::filesDropped,         this, &Tab::addAttachments);
  connect(ui->attachments->selectionModel(), &QItemSelectionModel::selectionChanged,           this, &Tab::onAttachmentSelectionChanged);
  connect(m_addAttachmentsAction,            &QAction::triggered,                              this, &Tab::onAddAttachments);
  connect(m_removeAttachmentsAction,         &QAction::triggered,                              this, &Tab::onRemoveAttachments);
  connect(ui->moveAttachmentsUp,             &QPushButton::clicked,                            this, &Tab::onMoveAttachmentsUp);
  connect(ui->moveAttachmentsDown,           &QPushButton::clicked,                            this, &Tab::onMoveAttachmentsDown);
  connect(ui->attachments,                   &Util::BasicTreeView::ctrlUpPressed,              this, &Tab::onMoveAttachmentsUp);
  connect(ui->attachments,                   &Util::BasicTreeView::ctrlDownPressed,            this, &Tab::onMoveAttachmentsDown);
  connect(ui->attachments,                   &Util::BasicTreeView::customContextMenuRequested, this, &Tab::showAttachmentsContextMenu);

  onAttachmentSelectionChanged();
}

void
Tab::withSelectedAttachments(std::function<void(Attachment *)> code) {
  if (m_currentlySettingInputControlValues)
    return;

  for (auto &indexRange : ui->attachments->selectionModel()->selection()) {
    auto idxs = indexRange.indexes();
    if (idxs.isEmpty() || !idxs.at(0).isValid())
      continue;

    auto attachment = m_attachmentsModel->attachmentForRow(idxs.at(0).row());
    if (attachment) {
      code(attachment.get());
      m_attachmentsModel->attachmentUpdated(*attachment);
    }
  }
}

QList<Attachment *>
Tab::selectedAttachments()
  const {
  auto attachments = QList<Attachment *>{};
  Util::withSelectedIndexes(ui->attachments, [&attachments, this](QModelIndex const &idx) {
    auto attachment = m_attachmentsModel->attachmentForRow(idx.row());
    if (attachment)
      attachments << attachment.get();
  });

  return attachments;
}

void
Tab::selectAttachments(QList<Attachment *> const &attachments) {
  auto numColumns = m_attachmentsModel->columnCount() - 1;
  auto selection  = QItemSelection{};

  for (auto const &attachment : attachments) {
    auto row = m_attachmentsModel->rowForAttachment(*attachment);
    selection.select(m_attachmentsModel->index(row, 0), m_attachmentsModel->index(row, numColumns));
  }

  ui->attachments->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
}

void
Tab::onAttachmentNameEdited(QString newValue) {
  withSelectedAttachments([&](Attachment *attachment) { attachment->m_name = newValue; });
}

void
Tab::onAttachmentDescriptionEdited(QString newValue) {
  withSelectedAttachments([&](Attachment *attachment) { attachment->m_description = newValue; });
}

void
Tab::onAttachmentMIMETypeEdited(QString newValue) {
  withSelectedAttachments([&](Attachment *attachment) { attachment->m_MIMEType = newValue; });
}

void
Tab::onAttachmentStyleChanged(int newValue) {
  auto data = ui->attachmentStyle->itemData(newValue);
  if (!data.isValid())
    return;

  auto style = data.toInt() == Attachment::ToAllFiles ? Attachment::ToAllFiles : Attachment::ToFirstFile;
  withSelectedAttachments([&](Attachment *attachment) { attachment->m_style = style; });
}

void
Tab::addAttachments(QStringList const &fileNames) {
  QList<AttachmentPtr> attachmentsToAdd;
  for (auto &fileName : fileNames) {
    attachmentsToAdd << std::make_shared<Attachment>(fileName);
    attachmentsToAdd.back()->guessMIMEType();
  }

  m_attachmentsModel->addAttachments(attachmentsToAdd);
  resizeAttachmentsColumnsToContents();
}

void
Tab::onAddAttachments() {
  auto fileNames = selectAttachmentsToAdd();
  if (!fileNames.isEmpty())
    addAttachments(fileNames);
}

QStringList
Tab::selectAttachmentsToAdd() {
  QFileDialog dlg{this};
  dlg.setNameFilter(QY("All files") + Q(" (*)"));
  dlg.setFileMode(QFileDialog::ExistingFiles);
  dlg.setDirectory(Util::Settings::get().m_lastOpenDir);
  dlg.setWindowTitle(QY("Add attachments"));

  if (!dlg.exec())
    return QStringList{};

  Util::Settings::get().m_lastOpenDir = dlg.directory();

  return dlg.selectedFiles();
}

void
Tab::onRemoveAttachments() {
  m_attachmentsModel->removeSelectedAttachments(ui->attachments->selectionModel()->selection());

  onAttachmentSelectionChanged();
}

void
Tab::resizeAttachmentsColumnsToContents()
  const {
  Util::resizeViewColumnsToContents(ui->attachments);
}

void
Tab::onAttachmentSelectionChanged() {
  auto selection = ui->attachments->selectionModel()->selection();
  auto numRows   = Util::numSelectedRows(selection);

  ui->moveAttachmentsUp->setEnabled(!!numRows);
  ui->moveAttachmentsDown->setEnabled(!!numRows);

  if (!numRows) {
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

  auto attachment = m_attachmentsModel->attachmentForRow(idxs.at(0).row());
  if (!attachment)
    return;

  setAttachmentControlValues(attachment.get());
}

void
Tab::enableAttachmentControls(bool enable) {
  auto controls = std::vector<QWidget *>{ui->attachmentName,     ui->attachmentNameLabel,     ui->attachmentDescription, ui->attachmentDescriptionLabel,
                                         ui->attachmentMIMEType, ui->attachmentMIMETypeLabel, ui->attachmentStyle,       ui->attachmentStyleLabel,};
  for (auto &control : controls)
    control->setEnabled(enable);
}

void
Tab::enableAttachmentsActions() {
  auto hasSelection = !ui->attachments->selectionModel()->selection().isEmpty();

  m_removeAttachmentsAction->setEnabled(hasSelection);
}

void
Tab::setAttachmentControlValues(Attachment *attachment) {
  m_currentlySettingInputControlValues = true;

  if (!attachment && ui->attachmentStyle->itemData(0).isValid())
    ui->attachmentStyle->insertItem(0, QY("<do not change>"));

  else if (attachment && !ui->attachmentStyle->itemData(0).isValid())
    ui->attachmentStyle->removeItem(0);

  if (!attachment) {
    ui->attachmentName->setText(Q(""));
    ui->attachmentDescription->setText(Q(""));
    ui->attachmentMIMEType->setEditText(Q(""));
    ui->attachmentStyle->setCurrentIndex(0);

  } else {
    ui->attachmentName->setText(        attachment->m_name);
    ui->attachmentDescription->setText( attachment->m_description);
    ui->attachmentMIMEType->setEditText(attachment->m_MIMEType);

    Util::setComboBoxIndexIf(ui->attachmentStyle, [&](QString const &, QVariant const &data) { return data.isValid() && (data.toInt() == static_cast<int>(attachment->m_style)); });
  }

  m_currentlySettingInputControlValues = false;
}

void
Tab::retranslateAttachmentsUI() {
  m_attachmentsModel->retranslateUi();
  resizeAttachmentsColumnsToContents();

  m_addAttachmentsAction->setText(QY("&Add"));
  m_removeAttachmentsAction->setText(QY("&Remove"));

  // Attachment style
  ui->attachmentStyle->setItemData(0, static_cast<int>(Attachment::ToAllFiles));
  ui->attachmentStyle->setItemData(1, static_cast<int>(Attachment::ToFirstFile));

  setupAttachmentsToolTips();
}

void
Tab::setupAttachmentsToolTips() {
  Util::setToolTip(ui->attachments, QY("Right-click to add and remove attachments"));
}

void
Tab::moveAttachmentsUpOrDown(bool up) {
  auto attachments = selectedAttachments();

  m_attachmentsModel->moveAttachmentsUpOrDown(attachments, up);

  selectAttachments(attachments);
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
  enableAttachmentsActions();
  m_attachmentsMenu->exec(ui->attachments->viewport()->mapToGlobal(pos));
}

}}}
