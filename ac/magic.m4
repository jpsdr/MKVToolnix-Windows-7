dnl
dnl Check for libmagic
dnl

AC_ARG_ENABLE([magic],
  AC_HELP_STRING([--enable-magic],[file type detection via libmagic (yes)]),
  [],[enable_magic=yes])

use_magic=no

if test x"$enable_magic" = xyes; then
  magic_mingw_libs=""
  if test "x$ac_cv_mingw32" = "xyes"; then
    magic_mingw_libs="-lshlwapi"
  fi

  PKG_CHECK_EXISTS([libmagic],[libmagic_pkgfound=yes],[libmagic_pkgfound=no])

  if test x"$libmagic_pkgfound" = xyes; then
    PKG_CHECK_MODULES([MAGIC],[libmagic],[libmagic_pkgfound=yes],[libmagic_pkgfound=no])
    MAGIC_LIBS="`$PKG_CONFIG --libs libmagic` $magic_mingw_libs"
    AC_DEFINE(HAVE_MAGIC_H, 1, [Define to 1 if you have the <magic.h> header file.])
    use_magic=yes

  else
    AC_CHECK_LIB(magic, magic_open, [ magic_found=yes ], [ magic_found=no ], [-lz $GNURX_LIBS $magic_mingw_libs])

    if test "x$magic_found" = "xyes"; then
      AC_CHECK_HEADERS([magic.h])
    fi

    if test "x$ac_cv_header_magic_h" = "xyes" ; then
      MAGIC_LIBS="-lmagic -lz $GNURX_LIBS $magic_mingw_libs"
      use_magic=yes
    fi
  fi
fi

if test $use_magic = yes; then
  opt_features_yes="$opt_features_yes\n   * libMagic file type detection"
else
  opt_features_no="$opt_features_no\n   * libMagic file type detection"
fi

AC_SUBST(MAGIC_LIBS)
