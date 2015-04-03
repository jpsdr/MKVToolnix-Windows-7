#include "common/common_pch.h"

#include "common/qt.h"
#include "mkvtoolnix-gui/header_editor/page_base.h"

namespace mtx { namespace gui { namespace HeaderEditor {

using namespace mtx::gui;

PageBase::PageBase(Tab &parent,
                   translatable_string_c const &title)
  : QWidget{parent.getPageContainer()}
  , m_parent{parent}
  , m_title{title}
{
  setVisible(false);

  setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Expanding});
}

PageBase::~PageBase() {
}

bool
PageBase::hasBeenModified() {
  if (hasThisBeenModified())
    return true;

  for (auto child : m_children)
    if (child->hasBeenModified())
      return true;

  return false;
}

void
PageBase::doModifications() {
  modifyThis();

  for (auto child : m_children)
    child->doModifications();
}

QModelIndex
PageBase::validate() {
  if (!validateThis())
    return m_pageIdx;

  for (auto child : m_children) {
    auto result = child->validate();
    if (result.isValid())
      return result;
  }

  return QModelIndex{};
}

QString
PageBase::getTitle() {
  return Q(m_title.get_translated());
}

}}}
