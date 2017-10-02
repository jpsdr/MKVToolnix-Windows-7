#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/header_editor/empty_page.h"

namespace mtx { namespace gui { namespace HeaderEditor {

class TopLevelPage: public EmptyPage {
  Q_OBJECT;
  QString m_internalIdentifier;
  QModelIndex m_parentPageIdx;

public:
  TopLevelPage(Tab &parent, translatable_string_c const &title, bool customLayout = false);
  virtual ~TopLevelPage();

  virtual void init();
  virtual void setParentPage(PageBase &page);
  virtual QString internalIdentifier() const;
  virtual void setInternalIdentifier(QString const &identifier);
};

}}}
