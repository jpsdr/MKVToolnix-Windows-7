#include "common/common_pch.h"

#include <QDateTime>
#include <QString>

#include "common/qt.h"
#include "mkvtoolnix-gui/util/date_time.h"

namespace mtx::gui::Util {

QString
displayableDate(QDateTime const &date) {
  if (!date.isValid())
    return {};

  return Q("%1 %2")
    .arg(date.toString(QString{"yyyy-MM-dd hh:mm:ss"}))
    .arg(timeZoneOffsetString(date));
}

QString
timeZoneOffsetString(QDateTime const &date) {
  if (date.timeSpec() == Qt::UTC)
    return Q("UTC");

  auto offset = date.offsetFromUtc();
  auto mult   = offset < 0 ? -1 : 1;

  return Q(fmt::format("{0}{1:02}:{2:02}", offset < 0 ? Q("-") : Q("+"), (offset * mult) / 60 / 60, (offset * mult / 60) % 60));
}

}
