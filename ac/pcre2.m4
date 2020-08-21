dnl
dnl Check for pcre2
dnl

PKG_CHECK_EXISTS([libpcre2-8],[libpcre2_8_found=yes],[libpcre2_8_found=no])
if test x"$libpcre2_8_found" = xyes; then
  PKG_CHECK_MODULES([PCRE2],[libpcre2-8],[libpcre2_8_found=yes])
  PCRE2_CFLAGS="`$PKG_CONFIG --cflags libpcre2-8`"
  PCRE2_LIBS="`$PKG_CONFIG --libs libpcre2-8`"
fi

if test x"$libpcre2_8_found" != xyes; then
  AC_MSG_ERROR([Could not find the PCRE2 library])
fi

AC_SUBST(PCRE2_CFLAGS)
AC_SUBST(PCRE2_LIBS)
