/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   terminal access functions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/os.h"

#if defined(HAVE_TIOCGWINSZ)
# include <termios.h>
# if defined(HAVE_SYS_IOCTL_H) || defined(GWINSZ_IN_SYS_IOCTL)
#  include <sys/ioctl.h>
# endif // HAVE_SYS_IOCTL_H || GWINSZ_IN_SYS_IOCTL
# if defined(HAVE_UNISTD_H)
#  include <unistd.h>
# endif // HAVE_UNISTD_H
# if defined(HAVE_STROPTS_H)
#  include <stropts.h>
# endif // HAVE_STROPTS_H
#endif  // HAVE_TIOCGWINSZ

#include "common/terminal.h"

constexpr auto DEFAULT_TERMINAL_COLUMNS = 80;

int
get_terminal_columns() {
#if defined(HAVE_TIOCGWINSZ)
  struct winsize ws;

  if (ioctl(0, TIOCGWINSZ, &ws) != 0)
    return DEFAULT_TERMINAL_COLUMNS;
  return ws.ws_col;

#else  // HAVE_TIOCGWINSZ
  return DEFAULT_TERMINAL_COLUMNS;
#endif  // HAVE_TIOCGWINSZ
}
