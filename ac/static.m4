dnl
dnl Check for static
dnl

AC_ARG_ENABLE([static], AC_HELP_STRING([--enable-static],[build static libraries (no)]), [], [enable_static=no])

STATIC_LIBS=""

if test x"$enable_static" = xyes ; then
  STATIC_LIBS="  -lpthread -static  "
  opt_features_yes="$opt_features_yes\n   * build static libraries"
else
  opt_features_no="$opt_features_no\n   * build static libraries"
fi

AC_SUBST(STATIC_LIBS)
