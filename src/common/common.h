/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   definitions used in all programs, helper functions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#undef min
#undef max

#include "common/os.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include <stdint.h>

#include <fmt/format.h>
#include <fmt/ostream.h>
#if FMT_VERSION >= 110000
# include <fmt/ranges.h>
#endif // FMT_VERSION >= 110000

#include <boost/algorithm/string.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/gmp.hpp>
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>

namespace balg = boost::algorithm;

using namespace std::string_literals;

#include <ebml/EbmlTypes.h>
#include <matroska/KaxTypes.h>
#undef min
#undef max

#include <ebml/EbmlElement.h>
#include <ebml/EbmlMaster.h>
#include <ebml/EbmlVersion.h>
#if LIBEBML_VERSION < 0x020000
# include <ebml/c/libebml_t.h>
#endif
#include <matroska/KaxVersion.h>

/* i18n stuff */
#if defined(HAVE_LIBINTL_H)
# include <libintl.h>
// libintl defines 'snprintf' to 'libintl_snprintf' on certain
// platforms such as mingw or macOS.  'std::snprintf' becomes
// 'std::libintl_snprintf' which doesn't exist.
# undef fprintf
# undef snprintf
# undef sprintf
#else
# define gettext(s)                            (s)
# define ngettext(s_singular, s_plural, count) ((count) != 1 ? (s_plural) : (s_singular))
#endif

#undef Y
#undef NY
#undef FY
#undef FNY
#define Y(s)                             gettext(s)
#define FY(s)                            fmt::runtime(gettext(s))
#define NY(s_singular, s_plural, count)  ngettext(s_singular, s_plural, count)
#define FNY(s_singular, s_plural, count) fmt::runtime(ngettext(s_singular, s_plural, count))

#define BUGMSG fmt::format(FY("This should not have happened. Please file an issue at " \
                              "{0} with this error/warning " \
                              "message, a description of what you were trying to do, " \
                              "the command line used and which operating system you are " \
                              "using. Thank you."), MTX_URL_ISSUES)

namespace mtx {
constexpr uint32_t calc_fourcc(char a, char b, char c, char d) {
  return (static_cast<uint32_t>(a) << 24)
       + (static_cast<uint32_t>(b) << 16)
       + (static_cast<uint32_t>(c) <<  8)
       +  static_cast<uint32_t>(d);
}
}

constexpr auto TIMESTAMP_SCALE = 1'000'000;

void mxrun_before_exit(std::function<void()> function);
[[noreturn]]
void mxexit(int code = -1);

extern unsigned int verbose;

void mtx_common_init(std::string const &program_name, char const *argv0);
std::string const &get_program_name();

#define MTX_DECLARE_PRIVATE(PrivateClass) \
  inline PrivateClass* p_func() { return reinterpret_cast<PrivateClass *>(&(*p_ptr)); } \
  inline const PrivateClass* p_func() const { return reinterpret_cast<const PrivateClass *>(&(*p_ptr)); } \
  friend class PrivateClass;

#define MTX_DECLARE_PUBLIC(Class)                                    \
  inline Class* q_func() { return static_cast<Class *>(q_ptr); } \
  inline const Class* q_func() const { return static_cast<const Class *>(q_ptr); } \
  friend class Class;

#if LIBEBML_VERSION >= 0x020000
# define MTX_EBML_IOCALLBACK_READ_RETURN_TYPE std::size_t
#else
# define MTX_EBML_IOCALLBACK_READ_RETURN_TYPE std::uint32_t

namespace libebml {
using open_mode                 = ::open_mode;
constexpr open_mode MODE_READ   = ::open_mode::MODE_READ;
constexpr open_mode MODE_WRITE  = ::open_mode::MODE_WRITE;
constexpr open_mode MODE_CREATE = ::open_mode::MODE_CREATE;
constexpr open_mode MODE_SAFE   = ::open_mode::MODE_SAFE;
}

#endif

#include "common/debugging.h"
#include "common/error.h"
#include "common/memory.h"
#include "common/mm_io.h"
#include "common/output.h"
