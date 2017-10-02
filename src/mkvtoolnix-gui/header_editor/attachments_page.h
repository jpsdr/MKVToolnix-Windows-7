#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/header_editor/top_level_page.h"
#include "mkvtoolnix-gui/util/files_drag_drop_handler.h"

namespace libmatroska {
class KaxAttached;
};

namespace mtx { namespace gui { namespace HeaderEditor {

namespace Ui {
class AttachmentsPage;
}

using KaxAttachedPtr  = std::shared_ptr<KaxAttached>;
using KaxAttachedList = QList< std::shared_ptr<KaxAttached> >;

class AttachmentsPage: public TopLevelPage {
  Q_OBJECT;

protected:
  std::unique_ptr<Ui::AttachmentsPage> ui;
  mtx::gui::Util::FilesDragDropHandler m_filesDDHandler;
  KaxAttachedList m_initialAttachments;

public:
  AttachmentsPage(Tab &parent, KaxAttachedList const &attachments);
  virtual ~AttachmentsPage();

  virtual void init() override;
  virtual bool hasThisBeenModified() const override;
  virtual QString internalIdentifier() const override;

signals:
  void filesDropped(QStringList const &fileNames);

public slots:
  virtual void retranslateUi() override;

protected:
  virtual void dragEnterEvent(QDragEnterEvent *event) override;
  virtual void dropEvent(QDropEvent *event) override;
};

}}}
