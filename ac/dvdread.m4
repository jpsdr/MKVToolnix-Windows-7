dnl
dnl Check for dvdread
dnl

AC_ARG_WITH([dvdread], AS_HELP_STRING([--without-dvdread],[do not build with libdvdread for reading chapters from DVDs]),
            [ with_dvdread=${withval} ], [ with_dvdread=yes ])
if test "x$with_dvdread" != "xno"; then
  PKG_CHECK_EXISTS([dvdread],[dvdread_found=yes],[dvdread_found=no])
  if test x"$dvdread_found" = xyes; then
    PKG_CHECK_MODULES([dvdread],[dvdread],[dvdread_found=yes])
    DVDREAD_CFLAGS="`$PKG_CONFIG --cflags dvdread`"
    DVDREAD_LIBS="`$PKG_CONFIG --libs dvdread`"
  fi
fi

if test x"$dvdread_found" = xyes; then
  AC_DEFINE(HAVE_DVDREAD,,[define if building with dvdread])
  USE_DVDREAD=yes
  opt_features_yes="$opt_features_yes\n   * DVD chapter support via libdvdread"
else
  opt_features_no="$opt_features_no\n   * DVD chapter support via libdvdread"
fi

AC_SUBST(DVDREAD_CFLAGS)
AC_SUBST(DVDREAD_LIBS)
AC_SUBST(USE_DVDREAD)
