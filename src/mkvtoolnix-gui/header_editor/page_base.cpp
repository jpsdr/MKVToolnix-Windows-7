#include "common/common_pch.h"

// #include <QDebug>
// #include <typeinfo>

#include "common/qt.h"
#include "mkvtoolnix-gui/header_editor/page_base.h"
#include "mkvtoolnix-gui/header_editor/value_page.h"

namespace mtx { namespace gui { namespace HeaderEditor {

using namespace mtx::gui;

PageBase::PageBase(Tab &parent,
                   translatable_string_c const &title)
  : QWidget{&parent}
  , m_parent(parent)
  , m_title{title}
{
  setVisible(false);

  setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Expanding});
}

PageBase::~PageBase() {
}

PageBase *
PageBase::hasBeenModified() {
  if (hasThisBeenModified()) {
    // auto vp = dynamic_cast<ValuePage const *>(this);
    // qDebug() << "I have been modified: " << typeid(*this).name() << " title " << title()
    //          << " orig " << (vp ? vp->originalValueAsString() : Q("<not a value page>")) << " current " << (vp ? vp->currentValueAsString() : Q("<not a value page>"));
    return this;
  }

  for (auto child : m_children) {
    auto modifiedPage = child->hasBeenModified();
    if (modifiedPage)
      return modifiedPage;
  }

  return nullptr;
}

void
PageBase::doModifications() {
  modifyThis();

  for (auto child : m_children)
    child->doModifications();
}

QModelIndex
PageBase::validate()
  const {
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
PageBase::title()
  const {
  return Q(m_title.get_translated());
}

void
PageBase::setItems(QList<QStandardItem *> const &items)
  const {
  items.at(0)->setText(title());
}

}}}
