#ifndef MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_VALUE_PAGE_H
#define MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_VALUE_PAGE_H

#include "common/common_pch.h"

#include "mkvtoolnix-gui/header_editor/page_base.h"

class QCheckBox;
class QLabel;
class QPushButton;

namespace mtx { namespace gui { namespace HeaderEditor {

class Tab;

class ValuePage: public PageBase {
  Q_OBJECT;

public:
  enum class ValueType {
    AsciiString,
    String,
    UnsignedInteger,
    Float,
    Binary,
    Bool,
  };

public:
  EbmlMaster &m_master;
  EbmlCallbacks const &m_callbacks;

  translatable_string_c m_description;

  ValueType m_valueType;

  QCheckBox *m_cbAddOrRemove{};
  QWidget *m_input{};
  QPushButton *m_bReset{};
  QLabel *m_lTitle{}, *m_lTypeLabel{}, *m_lType{}, *m_lDescriptionLabel{}, *m_lDescription{}, *m_lStatusLabel{}, *m_lStatus{}, *m_lOriginalValueLabel{}, *m_lOriginalValue{}, *m_lValueLabel{};

  EbmlElement *m_element{};
  bool m_present{};

  PageBase &m_topLevelPage;

public:
  ValuePage(Tab &parent, PageBase &topLevelPage, EbmlMaster &master, EbmlCallbacks const &callbacks, ValueType valueType, translatable_string_c const &title, translatable_string_c const &description);
  virtual ~ValuePage();

  virtual void init();

  virtual QWidget *createInputControl() = 0;
  virtual QString getOriginalValueAsString() const = 0;
  virtual QString getCurrentValueAsString() const = 0;
  virtual void resetValue() = 0;
  virtual bool validateValue() const = 0;
  virtual void copyValueToElement() = 0;

  virtual bool hasThisBeenModified() const override;
  virtual bool validateThis() const override;
  virtual void modifyThis() override;
  virtual void retranslateUi() override;

public slots:
  virtual void onResetClicked();
  virtual void onAddOrRemoveChecked();
};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_HEADER_EDITOR_VALUE_PAGE_H
