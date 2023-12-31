#pragma once

#include "common/common_pch.h"

#include "common/bitvalue.h"
#include "mkvtoolnix-gui/header_editor/value_page.h"

class QLineEdit;

namespace mtx::gui::HeaderEditor {

class Tab;

class BitValuePage: public ValuePage {
public:
  QLineEdit *m_leValue{};
  mtx::bits::value_c m_originalValue;
  unsigned int m_bitLength;

public:
  BitValuePage(Tab &parent, PageBase &topLevelPage, libebml::EbmlMaster &master, libebml::EbmlCallbacks const &callbacks, translatable_string_c const &title, translatable_string_c const &description, unsigned int bitLength);
  virtual ~BitValuePage();

  virtual QWidget *createInputControl() override;
  virtual QString originalValueAsString() const override;
  virtual QString currentValueAsString() const override;
  virtual void resetValue() override;
  virtual bool validateValue() const override;
  virtual void copyValueToElement() override;

  virtual mtx::bits::value_c valueToBitvalue() const;
};

}
