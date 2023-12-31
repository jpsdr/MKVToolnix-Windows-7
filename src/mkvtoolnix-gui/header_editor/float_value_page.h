#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/header_editor/value_page.h"

class QLineEdit;

namespace mtx::gui::HeaderEditor {

class Tab;

class FloatValuePage: public ValuePage {
public:
  QLineEdit *m_leValue{};
  double m_originalValue{};

public:
  FloatValuePage(Tab &parent, PageBase &topLevelPage, libebml::EbmlMaster &master, libebml::EbmlCallbacks const &callbacks, translatable_string_c const &title, translatable_string_c const &description);
  virtual ~FloatValuePage();

  virtual QWidget *createInputControl() override;
  virtual QString originalValueAsString() const override;
  virtual QString currentValueAsString() const override;
  virtual double currentValue(double valueIfNotPresent) const;
  virtual void resetValue() override;
  virtual bool validateValue() const override;
  virtual void copyValueToElement() override;
};

}
