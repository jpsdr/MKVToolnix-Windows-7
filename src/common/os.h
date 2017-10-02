/*
   mkvtoolnix - A set of programs for manipulating Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Cross platform compatibility definitions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#undef __STRICT_ANSI__

#include "config.h"

// For PRId64 and PRIu64:
#define __STDC_FORMAT_MACROS

#if (defined(_WIN32) || defined(WIN32) || defined(__MINGW32__)) && !defined(__CYGWIN__)
# define SYS_WINDOWS
#elif defined(__APPLE__)
# define SYS_APPLE
#else
# define COMP_GCC
# define SYS_UNIX
# if defined(__bsdi__) || defined(__FreeBSD__)
#  define SYS_BSD
# elif defined(__sun) && defined(__SUNPRO_CC)
#  undef COMP_GCC
#  define COMP_SUNPRO
#  define SYS_SOLARIS
# elif defined(__sun) && defined(__SVR4)
#  define SYS_SOLARIS
# elif defined(__CYGWIN__)
#  define COMP_CYGWIN
#  define SYS_CYGWIN
# else
#  define SYS_LINUX
# endif
#endif

#if defined(SYS_WINDOWS)
# if defined __MINGW32__
#  define COMP_MINGW
# else
#  define COMP_MSC
#  define NOMINMAX
# endif
#endif

#if (defined(SYS_WINDOWS) && defined(_WIN64)) || (!defined(SYS_WINDOWS) && (defined(__x86_64__) || defined(__ppc64__)))
# define ARCH_64BIT
#else
# define ARCH_32BIT
#endif

#if defined(COMP_MSC)

# define strncasecmp _strnicmp
# define strcasecmp _stricmp
using int64_t = __int64;
using int32_t = __int32;
using int16_t = __int16;
using int8_t = __int8;
using uint64_t = unsigned __int64;
using uint32_t = unsigned __int32;
using uint16_t = unsigned __int16;
using uint8_t = unsigned __int8;

# define nice(a)
using ssize_t = _fsize_t;

#define PACKED_STRUCTURE
#else // COMP_MSC
#define PACKED_STRUCTURE   __attribute__((__packed__))
#endif // COMP_MSC

#if defined(COMP_MINGW) || defined(COMP_MSC)

# if defined(COMP_MINGW)
// mingw is a special case. It does have inttypes.h declaring
// PRId64 to be I64d, but it warns about "int type format, different
// type argument" if %I64d is used with a int64_t. The mx* functions
// convert %lld to %I64d on mingw/msvc anyway.
#  undef PRId64
#  define PRId64 "lld"
#  undef PRIu64
#  define PRIu64 "llu"
#  undef PRIx64
#  define PRIx64 "llx"
# else
// MSVC doesn't have inttypes, nor the PRI?64 defines.
#  define PRId64 "I64d"
#  define PRIu64 "I64u"
#  define PRIx64 "I64x"
# endif // defined(COMP_MINGW)

#endif // COMP_MINGW || COMP_MSC

#define LLD "%" PRId64
#define LLU "%" PRIu64

#if defined(HAVE_NO_INT64_T)
using int64_t = INT64_TYPE;
#endif
#if defined(HAVE_NO_UINT64_T)
using uint64_t = UINT64_TYPE;
#endif

#if defined(WORDS_BIGENDIAN) && (WORDS_BIGENDIAN == 1)
# define ARCH_BIGENDIAN
#else
# define ARCH_LITTLEENDIAN
#endif
