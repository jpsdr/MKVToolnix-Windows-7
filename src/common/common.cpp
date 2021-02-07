/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   helper functions, common variables

   Written by Moritz Bunkus <moritz@bunkus.org>.
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
#include <matroska/FileKax.h>

#include "common/fs_sys_helpers.h"
#include "common/hacks.h"
#include "common/iana_language_subtag_registry.h"
#include "common/iso639.h"
#include "common/iso3166.h"
#include "common/iso15924.h"
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

  mm_file_io_c::cleanup();

  matroska_done();
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

/** \brief Sets the priority mkvmerge runs with

   Depending on the OS different functions are used. On Unix like systems
   the process is being nice'd if priority is negative ( = less important).
   Only the super user can increase the priority, but you shouldn't do
   such work as root anyway.
   On Windows SetPriorityClass is used.

   \param priority A value between -2 (lowest priority) and 2 (highest
     priority)
 */
void
set_process_priority(int priority) {
#if defined(SYS_WINDOWS)
  static const struct {
    int priority_class, thread_priority;
  } s_priority_classes[5] = {
    { IDLE_PRIORITY_CLASS,         THREAD_PRIORITY_IDLE         },
    { BELOW_NORMAL_PRIORITY_CLASS, THREAD_PRIORITY_BELOW_NORMAL },
    { NORMAL_PRIORITY_CLASS,       THREAD_PRIORITY_NORMAL       },
    { ABOVE_NORMAL_PRIORITY_CLASS, THREAD_PRIORITY_ABOVE_NORMAL },
    { HIGH_PRIORITY_CLASS,         THREAD_PRIORITY_HIGHEST      },
  };

  // If the lowest priority should be used and we're on Vista or later
  // then use background priority. This also selects a lower I/O
  // priority.
  if ((-2 == priority) && (mtx::sys::get_windows_version() >= WINDOWS_VERSION_VISTA)) {
    SetPriorityClass(GetCurrentProcess(), PROCESS_MODE_BACKGROUND_BEGIN);
    SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);
    return;
  }

  SetPriorityClass(GetCurrentProcess(), s_priority_classes[priority + 2].priority_class);
  SetThreadPriority(GetCurrentThread(), s_priority_classes[priority + 2].thread_priority);

#else
  static const int s_nice_levels[5] = { 19, 2, 0, -2, -5 };

  // Avoid a compiler warning due to glibc having flagged 'nice' with
  // 'warn if return value is ignored'.
  if (!nice(s_nice_levels[priority + 2])) {
  }

# if defined(HAVE_SYSCALL) && defined(SYS_ioprio_set)
  if (-2 == priority)
    syscall(SYS_ioprio_set,
            1,        // IOPRIO_WHO_PROCESS
            0,        // current process/thread
            3 << 13); // I/O class 'idle'
# endif
#endif
}

void
mtx_common_init(std::string const &program_name,
                char const *argv0) {
#if defined(USE_DRMINGW)
  ExcHndlInit();
#endif

  mtx::sys::determine_path_to_current_executable(argv0 ? std::string{argv0} : std::string{});

  random_c::init();

  g_cc_local_utf8 = charset_converter_c::init("");

  init_common_output(true);

  s_program_name = program_name;

#if defined(SYS_WINDOWS)
  fix_windows_errormode();
#endif

  matroska_init();

  debugging_c::init();
  mtx::hacks::init();

  init_locales();

  mm_file_io_c::setup();
  init_common_output(false);

  mtx::iana::language_subtag_registry::init();
  mtx::iso639::init();
  mtx::iso3166::init();
  mtx::iso15924::init();
  stereo_mode_c::init();
}

std::string const &
get_program_name() {
  return s_program_name;
}
