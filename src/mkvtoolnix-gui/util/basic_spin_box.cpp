#include "common/common_pch.h"

#include "common/list_utils.h"
#include "mkvtoolnix-gui/util/basic_spin_box.h"
#include <optional>

namespace mtx::gui::Util {

using namespace mtx::gui;

BasicSpinBox::BasicSpinBox(QWidget *parent)
  : QSpinBox{parent}
{
}

BasicSpinBox::~BasicSpinBox() {
}

QString
BasicSpinBox::textFromValue(int value)
  const {
  static std::optional<bool> s_forceArabicNumerals;

  if (!s_forceArabicNumerals) {
    auto script = QLocale::system().script();

    s_forceArabicNumerals = mtx::included_in(script, QLocale::QLocale::HanScript, QLocale::HanWithBopomofoScript, QLocale::SimplifiedHanScript, QLocale::TraditionalHanScript);
  }

  if (s_forceArabicNumerals.value())
    return QString::number(value);

  return QSpinBox::textFromValue(value);
}

}
