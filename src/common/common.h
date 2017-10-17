/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   definitions used in all programs, helper functions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#undef min
#undef max

#include <type_traits>

#undef __STRICT_ANSI__

#include <algorithm>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <cassert>
#include <cstring>

#if defined(HAVE_SYS_TYPES_H)
# include <sys/types.h>
#endif // HAVE_SYS_TYPES_H
#if defined(HAVE_STDINT_H)
# include <stdint.h>
#endif // HAVE_STDINT_H
#if defined(HAVE_INTTYPES_H)
# include <inttypes.h>
#endif // HAVE_INTTYPES_H

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/function.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/math/common_factor.hpp>
#include <boost/range.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/indexed.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/numeric.hpp>
#include <boost/rational.hpp>
#include <boost/regex.hpp>
#include <boost/system/error_code.hpp>

namespace balg  = boost::algorithm;
namespace badap = boost::adaptors;
namespace bfs   = boost::filesystem;
namespace brng  = boost::range;

#include "common/os.h"

#include <ebml/c/libebml_t.h>
#undef min
#undef max

#include <ebml/EbmlElement.h>
#include <ebml/EbmlMaster.h>

/* i18n stuff */
#if defined(HAVE_LIBINTL_H)
# include <libintl.h>
#else
# define gettext(s)                            (s)
# define ngettext(s_singular, s_plural, count) ((count) != 1 ? (s_plural) : (s_singular))
#endif

#undef Y
#undef NY
#define Y(s)                            gettext(u8##s)
#define NY(s_singular, s_plural, count) ngettext(u8##s_singular, u8##s_plural, count)
#define YF(s)        Y(s)
#define NYF(s, p, c) NY(s, p, c)

#include "common/debugging.h"
#include "common/error.h"
#include "common/memory.h"
#include "common/mm_io.h"
#include "common/output.h"

namespace libebml {
class EbmlBinary;
};

namespace libmatroska { };

using namespace libebml;
using namespace libmatroska;

#define BUGMSG Y("This should not have happened. Please contact the author " \
                 "Moritz Bunkus <moritz@bunkus.org> with this error/warning " \
                 "message, a description of what you were trying to do, " \
                 "the command line used and which operating system you are " \
                 "using. Thank you.")


#define MXMSG_ERROR    5
#define MXMSG_WARNING 10
#define MXMSG_INFO    15
#define MXMSG_DEBUG   20

#if !defined(FOURCC)
#define FOURCC(a, b, c, d) (uint32_t)((((unsigned char)a) << 24) + \
                                      (((unsigned char)b) << 16) + \
                                      (((unsigned char)c) <<  8) + \
                                       ((unsigned char)d))
#endif
#define isblanktab(c) (((c) == ' ')  || ((c) == '\t'))
#define iscr(c)       (((c) == '\n') || ((c) == '\r'))

#define TIMESTAMP_SCALE 1000000

void mxrun_before_exit(std::function<void()> function);
void mxexit(int code = -1);
void set_process_priority(int priority);

extern unsigned int verbose;

void mtx_common_init(std::string const &program_name, char const *argv0);
std::string const &get_program_name();
