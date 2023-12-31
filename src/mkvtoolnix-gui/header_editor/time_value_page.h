#pragma once

#include "common/common_pch.h"

#include <QDateTime>

#include "mkvtoolnix-gui/header_editor/value_page.h"

class QDateTimeEdit;

namespace mtx::gui::HeaderEditor {

class Tab;

class TimeValuePage: public ValuePage {
public:
  QDateTimeEdit *m_dteValue{};
  QDateTime m_originalValueUTC;

public:
  TimeValuePage(Tab &parent, PageBase &topLevelPage, libebml::EbmlMaster &master, libebml::EbmlCallbacks const &callbacks, translatable_string_c const &title, translatable_string_c const &description);
  virtual ~TimeValuePage();

  virtual QWidget *createInputControl() override;
  virtual QString originalValueAsString() const override;
  virtual QString currentValueAsString() const override;
  virtual QString note() const override;
  virtual void resetValue() override;
  virtual bool validateValue() const override;
  virtual void copyValueToElement() override;
  virtual void showInRequestedTimeSpec();
};

}
