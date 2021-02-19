#include "common/common_pch.h"

#include "common/path.h"
#include "common/strings/utf8.h"

namespace mtx::fs {

std::filesystem::path
to_path(std::string const &name) {
#if defined(SYS_WINDOWS)
  return std::filesystem::path{to_wide(name)};
#else
  return std::filesystem::path{name};
#endif
}

std::filesystem::path
to_path(std::wstring const &name) {
#if defined(SYS_WINDOWS)
  return std::filesystem::path{name};
#else
  return std::filesystem::path{to_utf8(name)};
#endif
}

}
