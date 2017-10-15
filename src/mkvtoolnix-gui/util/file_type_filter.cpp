#include "common/common_pch.h"

#include "common/file_types.h"
#include "common/qt.h"
#include "mkvtoolnix-gui/util/file_type_filter.h"

#include <QStringList>

namespace mtx { namespace gui { namespace Util {

QStringList FileTypeFilter::s_filter;

QStringList const &
FileTypeFilter::get() {
  if (!s_filter.isEmpty())
    return s_filter;

  auto &file_types = mtx::file_type_t::get_supported();

  std::map<QString, bool> all_extensions_map;
  for (auto &file_type : file_types) {
    auto extensions = to_qs(file_type.extensions).split(" ");
    QStringList extensions_full;

    for (auto &extension : extensions) {
      all_extensions_map[extension] = true;
      extensions_full << QString{"*.%1"}.arg(extension);

#if !defined(SYS_WINDOWS)
      auto extension_upper = extension.toUpper();
      all_extensions_map[extension_upper] = true;
      if (extension_upper != extension)
        extensions_full << QString("*.%1").arg(extension_upper);
#endif  // !SYS_WINDOWS
    }

    s_filter << QString("%1 (%2)").arg(to_qs(file_type.title)).arg(extensions_full.join(" "));
  }

  QStringList all_extensions;
  for (auto &extension : all_extensions_map)
    all_extensions << QString("*.%1").arg(extension.first);

  s_filter << QString("%1 (*.bdmv)").arg(QY("Blu-ray index files"));
  all_extensions << Q("*.bdmv");

  s_filter.sort();
  all_extensions.sort();

  s_filter.push_front(Q("%1 (*)").arg(QY("All files")));
  s_filter.push_front(Q("%1 (%2)").arg(QY("All supported media files")).arg(all_extensions.join(" ")));

  return s_filter;
}

}}}
