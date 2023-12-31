#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/header_editor/page_base.h"

class QCheckBox;
class QLabel;
class QPushButton;

namespace mtx::gui::HeaderEditor {

class Tab;

class ValuePage: public PageBase {
  Q_OBJECT

public:
  enum class ValueType {
    AsciiString,
    String,
    UnsignedInteger,
    Float,
    Binary,
    Bool,
    Timestamp,
  };

public:
  libebml::EbmlMaster &m_master;
  libebml::EbmlCallbacks const &m_callbacks;

  translatable_string_c m_description;

  ValueType m_valueType;

  QCheckBox *m_cbAddOrRemove{};
  QWidget *m_input{};
  QPushButton *m_bReset{};
  QLabel *m_lTitle{}, *m_lTypeLabel{}, *m_lType{}, *m_lDescriptionLabel{}, *m_lDescription{}, *m_lStatusLabel{}, *m_lStatus{}, *m_lOriginalValueLabel{}, *m_lOriginalValue{}, *m_lValueLabel{}, *m_lNote{}, *m_lNoteLabel{};

  libebml::EbmlElement *m_element{};
  bool m_present{}, m_mayBeRemoved{};

  PageBase &m_topLevelPage;

public:
  ValuePage(Tab &parent, PageBase &topLevelPage, libebml::EbmlMaster &master, libebml::EbmlCallbacks const &callbacks, ValueType valueType, translatable_string_c const &title, translatable_string_c const &description);
  virtual ~ValuePage();

  virtual void init();

  virtual QWidget *createInputControl() = 0;
  virtual QString originalValueAsString() const = 0;
  virtual QString currentValueAsString() const = 0;
  virtual QString note() const;
  virtual void resetValue() = 0;
  virtual bool validateValue() const = 0;
  virtual void copyValueToElement() = 0;

  virtual bool willBePresent() const;
  virtual bool hasThisBeenModified() const override;
  virtual bool validateThis() const override;
  virtual void modifyThis() override;
  virtual void retranslateUi() override;

Q_SIGNALS:
  virtual void valueChanged();

public Q_SLOTS:
  virtual void onResetClicked();
  virtual void onAddOrRemoveChecked();

protected:
  virtual void setupNote();
};

}
