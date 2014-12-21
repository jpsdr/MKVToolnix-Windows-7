dnl
dnl Test for libEBML and libMatroska, and define MATROSKA_CFLAGS and MATROSKA_LIBS
dnl

EBML_MATROSKA_INTERNAL=no
PKG_CHECK_MODULES([EBML],[libebml >= 1.3.1],[],[EBML_MATROSKA_INTERNAL=yes])
PKG_CHECK_MODULES([MATROSKA],[libmatroska >= 1.4.2],[],[EBML_MATROSKA_INTERNAL=yes])

if test x"$EBML_MATROSKA_INTERNAL" = xyes; then
  EBML_CFLAGS="-Ilib/libebml"
  EBML_LIBS="-Llib/libebml/src -lebml"
  MATROSKA_CFLAGS="-Ilib/libmatroska"
  MATROSKA_LIBS="-Llib/libmatroska/src -lmatroska"
fi

AC_SUBST(EBML_CFLAGS)
AC_SUBST(EBML_LIBS)
AC_SUBST(MATROSKA_CFLAGS)
AC_SUBST(MATROSKA_LIBS)
AC_SUBST(EBML_MATROSKA_INTERNAL)
