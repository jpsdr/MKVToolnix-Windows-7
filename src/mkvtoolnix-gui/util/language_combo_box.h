#ifndef MTX_MKVTOOLNIX_GUI_UTIL_LANGUAGE_COMBO_BOX_H
#define MTX_MKVTOOLNIX_GUI_UTIL_LANGUAGE_COMBO_BOX_H

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

#endif  // MTX_MKVTOOLNIX_GUI_UTIL_LANGUAGE_COMBO_BOX_H
