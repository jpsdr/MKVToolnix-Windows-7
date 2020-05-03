dnl
dnl Check for dvdread
dnl

PKG_CHECK_EXISTS([dvdread],[dvdread_found=yes],[dvdread_found=no])
if test x"$dvdread_found" = xyes; then
  PKG_CHECK_MODULES([dvdread],[dvdread],[dvdread_found=yes])
  DVDREAD_CFLAGS="`$PKG_CONFIG --cflags dvdread`"
  DVDREAD_LIBS="`$PKG_CONFIG --libs dvdread`"
fi

if test x"$dvdread_found" = xyes; then
  USE_DVDREAD=yes
  opt_features_yes="$opt_features_yes\n   * DVD chapter support via libdvdread"
else
  opt_features_no="$opt_features_no\n   * DVD chapter support via libdvdread"
fi

AC_DEFINE(HAVE_DVDREAD,,[define if building with dvdread])

AC_SUBST(DVDREAD_CFLAGS)
AC_SUBST(DVDREAD_LIBS)
AC_SUBST(USE_DVDREAD)
