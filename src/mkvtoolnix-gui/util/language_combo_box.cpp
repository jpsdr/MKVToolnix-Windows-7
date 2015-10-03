#include "common/common_pch.h"

#include <QAbstractItemView>
#include <QStringList>

#include "common/qt.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/util/language_combo_box.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx { namespace gui { namespace Util {

LanguageComboBox::LanguageComboBox(QWidget *parent)
  : ComboBoxBase{parent}
{
}

LanguageComboBox::~LanguageComboBox() {
}

LanguageComboBox::LanguageComboBox(ComboBoxBasePrivate &d,
                                   QWidget *parent)
  : ComboBoxBase{d, parent}
{
}

ComboBoxBase &
LanguageComboBox::setup(bool withEmpty,
                        QString const &emptyTitle) {
  ComboBoxBase::setup(withEmpty, emptyTitle);

  auto separatorOffset = 0;

  if (withEmpty) {
    addItem(emptyTitle, Q(""));
    ++separatorOffset;
  }

  for (auto const &language : App::iso639Languages()) {
    if (language.second != Q("und"))
      continue;

    addItem(language.first, language.second);
    insertSeparator(1 + separatorOffset);
    separatorOffset += 2;
    break;
  }

  auto &commonLanguages = App::commonIso639Languages();
  if (!commonLanguages.empty()) {
    for (auto const &language : commonLanguages)
      addItem(language.first, language.second);

    insertSeparator(commonLanguages.size() + separatorOffset);
  }

  for (auto const &language : App::iso639Languages())
    addItem(language.first, language.second);

  view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  Util::fixComboBoxViewWidth(*this);

  return *this;
}

}}}
