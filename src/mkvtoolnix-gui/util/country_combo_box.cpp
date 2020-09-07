#include "common/common_pch.h"

#include <QAbstractItemView>
#include <QStringList>

#include "common/qt.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/util/country_combo_box.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx::gui::Util {

CountryComboBox::CountryComboBox(QWidget *parent)
  : ComboBoxBase{parent}
{
}

CountryComboBox::CountryComboBox(ComboBoxBasePrivate &d,
                                 QWidget *parent)
  : ComboBoxBase{d, parent}
{
}

CountryComboBox::~CountryComboBox() {
}

bool
CountryComboBox::onlyShowOftenUsed()
  const {
  auto &cfg = Util::Settings::get();
  return cfg.m_oftenUsedRegionsOnly && !cfg.m_oftenUsedRegions.isEmpty();
}

ComboBoxBase &
CountryComboBox::setup(bool withEmpty,
                       QString const &emptyTitle) {
  auto onlyOftenUsed = onlyShowOftenUsed();

  ComboBoxBase::setup(withEmpty, emptyTitle);

  if (withEmpty)
    addItem(emptyTitle, Q(""));

  auto commonRegions = onlyOftenUsed ? mergeCommonAndAdditionalItems(App::commonRegions(), App::regions(), additionalItems()) : App::commonRegions();

  if (!commonRegions.empty()) {
    for (auto const &country : commonRegions)
      addItem(country.first, country.second);

    if (!onlyOftenUsed)
      insertSeparator(commonRegions.size() + (withEmpty ? 1 : 0));
  }

  if (!onlyOftenUsed)
    for (auto const &country : App::topLevelDomainCountryCodes())
      addItem(country.first, country.second);

  view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  Util::fixComboBoxViewWidth(*this);

  return *this;
}

}
