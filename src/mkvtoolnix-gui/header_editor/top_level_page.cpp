#include "common/common_pch.h"

#include "mkvtoolnix-gui/header_editor/top_level_page.h"

namespace mtx { namespace gui { namespace HeaderEditor {

using namespace mtx::gui;

TopLevelPage::TopLevelPage(Tab &parent,
                           translatable_string_c const &title,
                           bool customLayout)
  : EmptyPage{parent, title, "", customLayout}
{
}

TopLevelPage::~TopLevelPage() {
}

void
TopLevelPage::init() {
  m_parent.appendPage(this);
}

}}}
