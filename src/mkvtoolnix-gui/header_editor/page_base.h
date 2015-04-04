#ifndef MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_PAGE_BASE_H
#define MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_PAGE_BASE_H

#include "common/common_pch.h"

#include <QAbstractItemModel>
#include <QList>

#include "common/ebml.h"
#include "common/qt_kax_analyzer.h"
#include "common/translation.h"
#include "mkvtoolnix-gui/header_editor/tab.h"

namespace mtx { namespace gui { namespace HeaderEditor {

using namespace libebml;

class Tab;

class PageBase: public QWidget {
  Q_OBJECT;

public:
  QList<PageBase *> m_children;
  Tab &m_parent;
  QModelIndex m_pageIdx;
  ebml_element_cptr m_l1Element;
  translatable_string_c m_title;

public:
  PageBase(Tab &parent, translatable_string_c const &title);
  virtual ~PageBase();

  virtual bool hasBeenModified() const;
  virtual bool hasThisBeenModified() const = 0;
  virtual void doModifications();
  virtual void modifyThis() = 0;
  virtual QModelIndex validate() const;
  virtual bool validateThis() const = 0;
  virtual void retranslateUi() = 0;
  virtual QString getTitle() const;
};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_PAGE_BASE_H
