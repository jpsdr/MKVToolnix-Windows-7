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

ComboBoxBase &
CountryComboBox::setWithAlphaCodesOnly(bool enable) {
  m_withAlphaCodesOnly = enable;
  return *this;
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
  QRegularExpression reNumeric{Q("^[0-9]+$")};
  auto onlyOftenUsed = onlyShowOftenUsed();

  ComboBoxBase::setup(withEmpty, emptyTitle);

  if (withEmpty)
    addItem(emptyTitle, Q(""));

  auto commonRegions = onlyOftenUsed ? mergeCommonAndAdditionalItems(App::commonRegions(), App::regions(), additionalItems()) : App::commonRegions();

  if (m_withAlphaCodesOnly) {
    auto idx = 0u;

    while (idx < commonRegions.size())
      if (commonRegions[idx].second.contains(reNumeric))
        commonRegions.erase(commonRegions.begin() + idx);
      else
        ++idx;
  }

  if (!commonRegions.empty()) {
    for (auto const &country : commonRegions)
      addItem(country.first, country.second);

    if (!onlyOftenUsed)
      insertSeparator(commonRegions.size() + (withEmpty ? 1 : 0));
  }

  if (!onlyOftenUsed)
    for (auto const &country : App::regions())
      if (!m_withAlphaCodesOnly || !country.second.contains(reNumeric))
        addItem(country.first, country.second);

  view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  Util::fixComboBoxViewWidth(*this);

  return *this;
}

}
