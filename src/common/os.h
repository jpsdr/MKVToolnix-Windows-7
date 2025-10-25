/*
   mkvtoolnix - A set of programs for manipulating Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Cross platform compatibility definitions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "config.h"

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

#if defined(COMP_MSC)

# define strncasecmp _strnicmp
# define strcasecmp _stricmp

#define PACKED_STRUCTURE
#else // COMP_MSC
#define PACKED_STRUCTURE   __attribute__((__packed__))
#endif // COMP_MSC
