#include "common/common_pch.h"

#include <QAbstractItemView>
#include <QStringList>

#include "common/qt.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/util/character_set_combo_box.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/util.h"

namespace mtx { namespace gui { namespace Util {

CharacterSetComboBox::CharacterSetComboBox(QWidget *parent)
  : ComboBoxBase{parent}
{
}

CharacterSetComboBox::~CharacterSetComboBox() {
}

ComboBoxBase &
CharacterSetComboBox::setup(bool withEmpty,
                        QString const &emptyTitle) {
  m_withEmpty          = withEmpty;
  m_emptyTitle         = emptyTitle;

  if (m_withEmpty)
    addItem(m_emptyTitle, Q(""));

  for (auto const &characterSet : App::characterSets())
    addItem(characterSet, characterSet);

  auto &cfg = Settings::get();
  if (!cfg.m_oftenUsedCharacterSets.isEmpty())
    insertSeparator(cfg.m_oftenUsedCharacterSets.count() + (withEmpty ? 1 : 0));

  view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  return *this;
}

}}}
