AC_DEFUN([AX_COMPILER_IS_CLANG],[
  AC_CACHE_CHECK([compiler is clang], [ac_cv_compiler_is_clang], [
    if $CXX --version | grep -q -i clang ; then
      ac_cv_compiler_is_clang=yes
    else
      ac_cv_compiler_is_clang=no
    fi
  ])
])

AX_COMPILER_IS_CLANG

if test x"$ac_cv_compiler_is_clang" = xyes; then
  USE_CLANG=yes
fi

AC_SUBST(USE_CLANG)
