#ifndef MTX_MKVTOOLNIX_GUI_UTIL_CHARACTER_SET_COMBO_BOX_H
#define MTX_MKVTOOLNIX_GUI_UTIL_CHARACTER_SET_COMBO_BOX_H

#include "common/common_pch.h"

#include "mkvtoolnix-gui/util/combo_box_base.h"

namespace mtx { namespace gui { namespace Util {

class CharacterSetComboBox: public ComboBoxBase {
  Q_OBJECT;

protected:
  explicit CharacterSetComboBox(ComboBoxBasePrivate &d, QWidget *parent);

public:
  explicit CharacterSetComboBox(QWidget *parent = nullptr);
  virtual ~CharacterSetComboBox();

  virtual ComboBoxBase &setup(bool withEmpty = false, QString const &emptyTitle = QString{}) override;

  virtual bool onlyShowOftenUsed() const override;
};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_UTIL_CHARACTER_SET_COMBO_BOX_H
