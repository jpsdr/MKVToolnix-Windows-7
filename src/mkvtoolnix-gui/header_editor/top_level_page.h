#ifndef MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_TOP_LEVEL_PAGE_H
#define MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_TOP_LEVEL_PAGE_H

#include "common/common_pch.h"

#include "mkvtoolnix-gui/header_editor/empty_page.h"

namespace mtx { namespace gui { namespace HeaderEditor {

class TopLevelPage: public EmptyPage {
  Q_OBJECT;
public:
  TopLevelPage(Tab &parent, translatable_string_c const &title, bool customLayout = false);
  virtual ~TopLevelPage();

  virtual void init();
};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_TOP_LEVEL_PAGE_H
