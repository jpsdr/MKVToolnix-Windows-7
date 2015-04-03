#include "common/common_pch.h"

#include "common/ebml.h"
#include "common/segmentinfo.h"
#include "mkvtoolnix-gui/header_editor/top_level_page.h"

namespace mtx { namespace gui { namespace HeaderEditor {

using namespace mtx::gui;

TopLevelPage::TopLevelPage(Tab &parent,
                           translatable_string_c const &title,
                           ebml_element_cptr const &l1Element)
  : EmptyPage{parent, title, ""}
{
  m_l1Element = l1Element;
}

TopLevelPage::~TopLevelPage() {
}

void
TopLevelPage::init() {
  m_parent.appendPage(this);
}

void
TopLevelPage::doModifications() {
  EmptyPage::doModifications();

  if (Is<KaxInfo>(m_l1Element.get()))
    fix_mandatory_segmentinfo_elements(m_l1Element.get());

  m_l1Element->UpdateSize(true, true);
}

}}}
