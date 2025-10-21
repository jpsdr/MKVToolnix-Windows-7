/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   helper functions, common variables

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#ifdef SYS_WINDOWS
# include <windows.h>
#endif
#if defined(HAVE_UNISTD_H)
# include <unistd.h>
#endif  // HAVE_UNISTD_H
#if defined(HAVE_SYS_SYSCALL_H)
# include <sys/syscall.h>
#endif

#include <matroska/KaxVersion.h>

#include "common/audio_emphasis.h"
#include "common/fs_sys_helpers.h"
#include "common/hacks.h"
#include "common/iana_language_subtag_registry.h"
#include "common/iso639.h"
#include "common/iso3166.h"
#include "common/iso15924.h"
#include "common/logger.h"
#include "common/mm_file_io.h"
#include "common/mm_stdio.h"
#include "common/random.h"
#include "common/stereo_mode.h"
#include "common/strings/editing.h"
#include "common/translation.h"

#if defined(USE_DRMINGW)
extern "C" {
# include "exchndl.h"
}
#endif

#if defined(SYS_WINDOWS)
#include "common/fs_sys_helpers.h"

// See http://msdn.microsoft.com/en-us/library/windows/desktop/ms686219(v=vs.85).aspx
# if !defined(PROCESS_MODE_BACKGROUND_BEGIN)
#  define PROCESS_MODE_BACKGROUND_BEGIN 0x00100000
# endif

// See http://msdn.microsoft.com/en-us/library/windows/desktop/ms686277(v=vs.85).aspx
# if !defined(THREAD_MODE_BACKGROUND_BEGIN)
#  define THREAD_MODE_BACKGROUND_BEGIN 0x00010000
# endif

static void
fix_windows_errormode() {
  UINT mode  = SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX;
  mode      |= GetErrorMode();

  ::SetErrorMode(mode);
}
#endif

// Global and static variables

unsigned int verbose = 1;

extern bool g_warning_issued;
static std::string s_program_name;

// Functions

static void
mtx_common_cleanup() {
  // Make sure g_mm_stdio is closed before the global destruction
  // kicks in. If it's redirected to a file then this is an instance
  // of a buffered file. If it's only collected via global destruction
  // after its reference count reaches zero then flushing the buffers
  // will call some debugging option stuff which is invalid at that
  // point.
  if (stdio_redirected()) {
    g_mm_stdio->close();
    g_mm_stdio = std::shared_ptr<mm_io_c>(new mm_stdio_c);
  }
}

static std::vector<std::function<void()> > s_to_run_before_exit;

void
mxrun_before_exit(std::function<void()> function) {
  s_to_run_before_exit.push_back(function);
}

void
mxexit(int code) {
  for (auto const &function : s_to_run_before_exit)
    function();

  mtx_common_cleanup();

  if (code != -1)
    exit(code);

  if (g_warning_issued)
    exit(1);

  exit(0);
}

void
mtx_common_init(std::string const &program_name,
                char const *argv0) {
#if defined(USE_DRMINGW)
  ExcHndlInit();
#endif

  debugging_c::init();
  mtx::log::init();
  random_c::init();

  initialize_std_and_boost_filesystem_locales();

  g_cc_local_utf8 = charset_converter_c::init("");

  mtx::sys::determine_path_to_current_executable(argv0 ? std::string{argv0} : std::string{});

  init_common_output(true);

  s_program_name = program_name;

#if defined(SYS_WINDOWS)
  fix_windows_errormode();
#endif

  mtx::hacks::init();

  init_locales();

  init_common_output(false);

  mtx::iana::language_subtag_registry::init();
  mtx::iso639::init();
  mtx::iso3166::init();
  mtx::iso15924::init();
  mtx::iana::language_subtag_registry::init_preferred_values(); // uses language_c::parse() & must therefore be initialized last
  audio_emphasis_c::init();
  stereo_mode_c::init();
}

std::string const &
get_program_name() {
  return s_program_name;
}
