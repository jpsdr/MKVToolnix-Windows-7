dnl
dnl Check for zlib
dnl

PKG_CHECK_EXISTS([zlib],[zlib_found=yes],[zlib_found=no])
if test x"$zlib_found" = xyes; then
  PKG_CHECK_MODULES([ZLIB],[zlib],[zlib_found=yes])
else
  AC_MSG_CHECKING(for ZLIB)
  save_LIBS="$LIBS"
  LIBS="$LIBS -lz"
  AC_LINK_IFELSE([AC_LANG_PROGRAM(
      [[#include <zlib.h>]],
      [[inflate(0, 0);]])],
    [zlib_found=yes; ZLIB_LIBS=-lz],[])
  LIBS="$save_LIBS"
  AC_MSG_RESULT($zlib_found)
fi

if test x"$zlib_found" != xyes; then
  AC_MSG_ERROR([Could not find the zlib library])
fi
