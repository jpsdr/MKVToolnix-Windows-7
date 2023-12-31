#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/header_editor/value_page.h"

class QLineEdit;

namespace mtx::gui::HeaderEditor {

class Tab;

class AsciiStringValuePage: public ValuePage {
public:
  QLineEdit *m_leValue{};
  std::string m_originalValue;

public:
  AsciiStringValuePage(Tab &parent, PageBase &topLevelPage, libebml::EbmlMaster &master, libebml::EbmlCallbacks const &callbacks, translatable_string_c const &title, translatable_string_c const &description);
  virtual ~AsciiStringValuePage();

  virtual QWidget *createInputControl() override;
  virtual QString originalValueAsString() const override;
  virtual QString currentValueAsString() const override;
  virtual void resetValue() override;
  virtual bool validateValue() const override;
  virtual void copyValueToElement() override;
};

}
