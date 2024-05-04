AC_DEFUN([AX_GTEST],[
  GTEST_CFLAGS=
  GTEST_LIBS=
  GTEST_TYPE=no

  CPPFLAGS_SAVED="$CPPFLAGS"
  AC_LANG_PUSH(C++)

  PKG_CHECK_EXISTS([gtest],[gtest_found=yes],[gtest_found=no])

  if test x"$gtest_found" = xyes; then
    PKG_CHECK_MODULES([gtest],[gtest],[gtest_found=yes])
    GTEST_CFLAGS="`$PKG_CONFIG --cflags gtest`"
    GTEST_LIBS="`$PKG_CONFIG --libs gtest`"
    GTEST_TYPE=system

  elif test -d lib/gtest/include && test -d lib/gtest/src ; then
    AC_MSG_CHECKING(for internal gtest)
    AC_CACHE_VAL(ax_cv_gtest_internal,[
      CPPFLAGS="$CPPFLAGS_SAVED -Ilib/gtest/include"
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <gtest/gtest.h>]], [[]])],[ax_cv_gtest_internal=yes],[ax_cv_gtest_internal=no])
    ])
    AC_MSG_RESULT($ax_cv_gtest_internal)

    if test x$ax_cv_gtest_internal=yes; then
      GTEST_TYPE=internal
    fi
  fi

  AC_LANG_POP
  CPPFLAGS="$CPPFLAGS_SAVED"

  AC_SUBST(GTEST_CFLAGS)
  AC_SUBST(GTEST_LIBS)
  AC_SUBST(GTEST_TYPE)
])

AX_GTEST
AC_PATH_PROG(VALGRIND, valgrind,, $PATH)
