#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/header_editor/value_page.h"

class QComboBox;

namespace mtx::gui::HeaderEditor {

class Tab;

class TrackNamePage: public ValuePage {
  Q_OBJECT

public:
  QComboBox *m_cbTrackName{};
  QString m_originalValue;
  uint64_t m_trackType{};

public:
  TrackNamePage(Tab &parent, PageBase &topLevelPage, EbmlMaster &master, EbmlCallbacks const &callbacks, translatable_string_c const &title, translatable_string_c const &description);
  virtual ~TrackNamePage();

  virtual QWidget *createInputControl() override;
  virtual QString originalValueAsString() const override;
  virtual QString currentValueAsString() const override;
  virtual void resetValue() override;
  virtual bool validateValue() const override;
  virtual void copyValueToElement() override;

  virtual void setString(QString const &value);

public Q_SLOTS:
  virtual void setupPredefinedTrackNames();
};

}
