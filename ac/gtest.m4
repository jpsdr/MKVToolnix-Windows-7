AC_DEFUN([AX_GTEST],[
  GTEST_TYPE=system

  CPPFLAGS_SAVED="$CPPFLAGS"
  AC_LANG_PUSH(C++)

  AC_CHECK_LIB([gtest_main],[main],[true],[GTEST_TYPE=no],[-lpthread])
  AC_CHECK_HEADERS([gtest/gtest.h],[true],[GTEST_TYPE=no])

  if test $GTEST_TYPE = no && test -d lib/gtest/include && test -d lib/gtest/src ; then
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

  AC_SUBST(GTEST_TYPE)
])

AX_GTEST
AC_PATH_PROG(VALGRIND, valgrind,, $PATH)
