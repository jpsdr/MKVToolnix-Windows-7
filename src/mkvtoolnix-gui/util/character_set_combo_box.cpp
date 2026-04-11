#include "common/common_pch.h"

#include <QAbstractItemView>
#include <QStringList>

#include "common/qt.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/util/character_set_combo_box.h"
#include "mkvtoolnix-gui/util/container.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx::gui::Util {

CharacterSetComboBox::CharacterSetComboBox(QWidget *parent)
  : ComboBoxBase{parent}
{
}

CharacterSetComboBox::CharacterSetComboBox(ComboBoxBasePrivate &d,
                                           QWidget *parent)
  : ComboBoxBase{d, parent}
{
}

CharacterSetComboBox::~CharacterSetComboBox() {
}

bool
CharacterSetComboBox::onlyShowOftenUsed()
  const {
  auto &cfg = Util::Settings::get();
  return cfg.m_oftenUsedCharacterSetsOnly && !cfg.m_oftenUsedCharacterSets.isEmpty();
}

ComboBoxBase &
CharacterSetComboBox::setup(bool withEmpty,
                            QString const &emptyTitle) {
  auto onlyOftenUsed = onlyShowOftenUsed();

  ComboBoxBase::setup(withEmpty, emptyTitle);

  // Block the underlying model's signals during batch population to
  // prevent O(n²) macOS accessibility element creation per addItem().
  auto itemModel       = model();
  auto modelWasBlocked = itemModel->blockSignals(true);

  if (withEmpty)
    addItem(emptyTitle, Q(""));

  auto commonCharacterSets = QStringList{ QStringList::fromVector(Util::stdVectorToQVector<QString>(App::commonCharacterSets())) };

  if (onlyOftenUsed) {
    auto merged  = Util::qListToSet(commonCharacterSets);
    merged      += Util::qListToSet(additionalItems());

    merged.remove(QString{});

    commonCharacterSets = QStringList{ merged.values() };
    commonCharacterSets.sort();
  }

  if (!commonCharacterSets.empty()) {
    for (auto const &characterSet : commonCharacterSets)
      addItem(characterSet, characterSet);

    if (!onlyOftenUsed)
      insertSeparator(commonCharacterSets.size() + (withEmpty ? 1 : 0));
  }

  if (!onlyOftenUsed)
    for (auto const &characterSet : App::characterSets())
      addItem(characterSet, characterSet);

  itemModel->blockSignals(modelWasBlocked);

  // Single view refresh after all items are added.
  view()->reset();
  view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  Util::fixComboBoxViewWidth(*this);

  return *this;
}

}
