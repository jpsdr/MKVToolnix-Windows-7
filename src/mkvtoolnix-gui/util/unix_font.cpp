#include "common/common_pch.h"

#include <QFont>

#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/util/font.h"

namespace mtx::gui::Util {

QFont
defaultUiFont() {
  return App::font();
}

}
