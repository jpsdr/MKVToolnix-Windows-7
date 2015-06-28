#include "common/common_pch.h"

#include <QAbstractItemView>
#include <QStringList>

#include "common/qt.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/util/language_combo_box.h"
#include "mkvtoolnix-gui/util/util.h"

namespace mtx { namespace gui { namespace Util {

LanguageComboBox::LanguageComboBox(QWidget *parent)
  : ComboBoxBase{parent}
{
}

LanguageComboBox::~LanguageComboBox() {
}

ComboBoxBase &
LanguageComboBox::setup(bool withEmpty,
                        QString const &emptyTitle) {
  m_withEmpty          = withEmpty;
  m_emptyTitle         = emptyTitle;

  auto separatorOffset = 0;

  if (m_withEmpty) {
    addItem(m_emptyTitle, Q(""));
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

  return *this;
}

}}}
