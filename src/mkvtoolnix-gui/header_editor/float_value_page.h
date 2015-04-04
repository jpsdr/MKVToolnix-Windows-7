#ifndef MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_FLOAT_VALUE_PAGE_H
#define MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_FLOAT_VALUE_PAGE_H

#include "common/common_pch.h"

#include "mkvtoolnix-gui/header_editor/value_page.h"

class QLineEdit;

namespace mtx { namespace gui { namespace HeaderEditor {

class Tab;

class FloatValuePage: public ValuePage {
public:
  QLineEdit *m_leValue{};
  double m_originalValue{};

public:
  FloatValuePage(Tab &parent, PageBase &topLevelPage, EbmlMaster &master, EbmlCallbacks const &callbacks, translatable_string_c const &title, translatable_string_c const &description);
  virtual ~FloatValuePage();

  virtual QWidget *createInputControl() override;
  virtual QString getOriginalValueAsString() const override;
  virtual QString getCurrentValueAsString() const override;
  virtual void resetValue() override;
  virtual bool validateValue() const override;
  virtual void copyValueToElement() override;
};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_FLOAT_VALUE_PAGE_H
