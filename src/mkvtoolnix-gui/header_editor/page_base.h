#pragma once

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
  translatable_string_c m_title;

public:
  PageBase(Tab &parent, translatable_string_c const &title);
  virtual ~PageBase();

  virtual PageBase *hasBeenModified();
  virtual bool hasThisBeenModified() const = 0;
  virtual void doModifications();
  virtual void modifyThis() = 0;
  virtual QModelIndex validate() const;
  virtual bool validateThis() const = 0;
  virtual void retranslateUi() = 0;
  virtual QString title() const;
  virtual void setItems(QList<QStandardItem *> const &items) const;
};

}}}
