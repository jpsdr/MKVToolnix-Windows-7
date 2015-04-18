#ifndef MTX_MKVTOOLNIX_GUI_UTIL_WINDOW_GEOMETRY_SAVER_H
#define MTX_MKVTOOLNIX_GUI_UTIL_WINDOW_GEOMETRY_SAVER_H

#include "common/common_pch.h"

#include <QString>

class QWidget;

namespace mtx { namespace gui { namespace Util {

class WindowGeometrySaver {
protected:
  QWidget *m_widget;
  QString m_name;

public:
  WindowGeometrySaver(QWidget *widget, QString const &name);
  ~WindowGeometrySaver();

  WindowGeometrySaver(WindowGeometrySaver const &)             = delete;
  WindowGeometrySaver &operator =(WindowGeometrySaver const &) = delete;

  void restore();
  void save() const;
};

}}}

#endif  // MTX_MKVTOOLNIX_GUI_UTIL_WINDOW_GEOMETRY_SAVER_H
