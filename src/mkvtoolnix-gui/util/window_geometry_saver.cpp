#include "common/common_pch.h"

#include <QDesktopWidget>
#include <QScreen>
#include <QSettings>

#include "common/qt.h"
#include "mkvtoolnix-gui/app.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/window_geometry_saver.h"

namespace mtx { namespace gui { namespace Util {

WindowGeometrySaver::WindowGeometrySaver(QWidget *widget,
                                         QString const &name)
  : m_widget{widget}
  , m_name{name}
{
}

WindowGeometrySaver::~WindowGeometrySaver() {
  save();
}

void
WindowGeometrySaver::restore() {
  auto reg = Util::Settings::registry();
  reg->beginGroup(Q("windowGeometry"));
  reg->beginGroup(m_name);

  // Position
  auto desktop = App::desktop();
  auto screens = App::screens();
  auto screen  = std::max(std::min<int>(reg->value(Q("screen"), desktop->primaryScreen()).toInt(), screens.count()), 0);
  auto rect    = desktop->screenGeometry(screen);

  auto v1      = reg->value(Q("x"));
  auto v2      = reg->value(Q("y"));

  if (v1.isValid() && v2.isValid() && (v1.toInt() >= 0) && (v2.toInt() >= 0))
    m_widget->move(std::min<int>(v1.toInt(), rect.x() + rect.width()  - 50),
                   std::min<int>(v2.toInt(), rect.y() + rect.height() - 32));

  // Size
  v1 = reg->value(Q("width"));
  v2 = reg->value(Q("height"));

  if (v1.isValid() && v2.isValid() && (v1.toInt() >= 0) && (v2.toInt() >= 0))
    m_widget->resize(std::max(std::min<int>(v1.toInt(), rect.width()),  100),
                     std::max(std::min<int>(v2.toInt(), rect.height()), 100));

  auto newState = m_widget->windowState() & ~Qt::WindowMinimized & ~Qt::WindowMaximized;
  if (reg->value(Q("minimized")).toBool())
    newState |= Qt::WindowMinimized;
  if (reg->value(Q("maximized")).toBool())
    newState |= Qt::WindowMaximized;
  m_widget->setWindowState(newState);

  reg->endGroup();
  reg->endGroup();
}

void
WindowGeometrySaver::save()
  const {
  auto reg = Util::Settings::registry();
  reg->beginGroup(Q("windowGeometry"));
  reg->beginGroup(m_name);

#if defined(SYS_WINDOWS) || defined(SYS_APPLE)
  auto pos  = m_widget->pos();
#else
  auto pos  = m_widget->mapToGlobal(QPoint{0, 0});
#endif
  auto size = m_widget->size();

  if ((pos.x() >= 0) && (pos.y() >= 0)) {
    reg->setValue(Q("x"), pos.x());
    reg->setValue(Q("y"), pos.y());
  }

  if ((size.width() > 100) && (size.height() > 100)) {
    reg->setValue(Q("width"),  size.width());
    reg->setValue(Q("height"), size.height());
  }

  reg->setValue(Q("minimized"), m_widget->isMinimized());
  reg->setValue(Q("maximized"), m_widget->isMaximized());
  reg->setValue(Q("screenn"),   App::desktop()->screenNumber(m_widget));

  reg->endGroup();
  reg->endGroup();
}

}}}
