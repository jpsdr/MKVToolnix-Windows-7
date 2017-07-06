#include "common/common_pch.h"

#include <QFont>

#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/util/widget.h"

namespace mtx { namespace gui { namespace Util {

QFont
defaultUiFont() {
  return App::font();
}

}}}
