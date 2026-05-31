dnl
dnl Miscellaneous tools: Inkscape & ImageMagick (required for Windows), Pandoc (only for development)
dnl
AC_PATH_PROG(CONVERT, convert)
AC_PATH_PROG(INKSCAPE, inkscape)
AC_PATH_PROG(MAGICK, magick)
AC_PATH_PROG(PANDOC, pandoc)

if test "x$ac_cv_mingw32" = "xyes"; then
  if test "x$ac_cv_path_INKSCAPE" = "x"; then
    AC_MSG_ERROR([Could not find the inkscape program])
  fi

  if test "x$ac_cv_path_MAGICK" = "x" -a "x$ac_cv_path_CONVERT" = "x"; then
    AC_MSG_ERROR([Could not find the ImageMagick's 'magick' or 'convert' programs])
  fi
fi
