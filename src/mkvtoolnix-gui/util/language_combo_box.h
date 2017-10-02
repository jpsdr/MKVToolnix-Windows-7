#pragma once

#include "common/common_pch.h"

#include "mkvtoolnix-gui/util/combo_box_base.h"

namespace mtx { namespace gui { namespace Util {

class LanguageComboBox: public ComboBoxBase {
  Q_OBJECT;

protected:
  explicit LanguageComboBox(ComboBoxBasePrivate &d, QWidget *parent);

public:
  explicit LanguageComboBox(QWidget *parent = nullptr);
  virtual ~LanguageComboBox();

  virtual ComboBoxBase &setup(bool withEmpty = false, QString const &emptyTitle = QString{}) override;

  virtual bool onlyShowOftenUsed() const;
};

}}}
