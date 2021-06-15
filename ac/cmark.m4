dnl
dnl Check for cmark
dnl

if test x"$BUILD_GUI" = xyes; then
  PKG_CHECK_EXISTS([libcmark],[cmark_found=yes],[cmark_found=no])
  if test x"$cmark_found" = xyes; then
    PKG_CHECK_MODULES([libcmark],[libcmark],[cmark_found=yes])
    CMARK_CFLAGS="`$PKG_CONFIG --cflags libcmark`"
    CMARK_LIBS="`$PKG_CONFIG --libs libcmark`"
  fi

  if test x"$cmark_found" != xyes; then
    AC_MSG_ERROR([Could not find the cmark library])
  fi

  AC_DEFINE(HAVE_CMARK,,[define if building with cmark])

else
  CMARK_CFLAGS=""
  CMARK_LIBS=""
fi

AC_SUBST(CMARK_CFLAGS)
AC_SUBST(CMARK_LIBS)
