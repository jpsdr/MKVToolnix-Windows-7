#include "common/common_pch.h"

#include <QDragEnterEvent>
#include <QDropEvent>

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/header_editor/attachments_page.h"
#include "mkvtoolnix-gui/forms/header_editor/tab.h"
#include "mkvtoolnix-gui/header_editor/attached_file_page.h"
#include "mkvtoolnix-gui/header_editor/attachments_page.h"

namespace mtx::gui::HeaderEditor {

using namespace mtx::gui;

AttachmentsPage::AttachmentsPage(Tab &parent,
                                 KaxAttachedList const &attachments)
  : TopLevelPage{parent, YT("Attachments"), true}
  , ui{new Ui::AttachmentsPage}
  , m_filesDDHandler{Util::FilesDragDropHandler::Mode::Remember}
  , m_initialAttachments{attachments}
{
  ui->setupUi(this);

  connect(ui->addAttachments, &QPushButton::clicked,          &parent, &Tab::selectAttachmentsAndAdd);
  connect(this,               &AttachmentsPage::filesDropped, &parent, &Tab::addAttachments);
}

AttachmentsPage::~AttachmentsPage() {
}

void
AttachmentsPage::init() {
  TopLevelPage::init();

  for (auto const &attachment : m_initialAttachments)
    m_parent.addAttachment(attachment);
}

bool
AttachmentsPage::hasThisBeenModified()
  const {
  auto numChildren = m_children.count();

  if (m_initialAttachments.count() != numChildren)
    return true;

  for (auto idx = 0; idx < numChildren; ++idx)
    if (m_initialAttachments[idx] != dynamic_cast<AttachedFilePage &>(*m_children[idx]).m_attachment)
      return true;

  return false;
}

void
AttachmentsPage::retranslateUi() {
  ui->retranslateUi(this);
}

void
AttachmentsPage::dragEnterEvent(QDragEnterEvent *event) {
  m_filesDDHandler.handle(event, false);
}

void
AttachmentsPage::dropEvent(QDropEvent *event) {
  if (m_filesDDHandler.handle(event, true))
    Q_EMIT filesDropped(m_filesDDHandler.fileNames());
}

QString
AttachmentsPage::internalIdentifier()
  const {
  return Q("attachments");
}

void
AttachmentsPage::rereadChildren(PageModel &model) {
  auto numRows = model.rowCount(m_pageIdx);

  m_children.clear();
  m_children.reserve(numRows);

  for (int row = 0; row < numRows; ++row)
    m_children.push_back(model.selectedPage(model.index(row, 0, m_pageIdx)));
}

}
