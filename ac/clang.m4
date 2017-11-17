AC_DEFUN([AX_COMPILER_IS_CLANG],[
  AC_CACHE_CHECK([compiler is clang], [ac_cv_compiler_is_clang], [
    if $CXX --version | grep -q -i clang ; then
      ac_cv_compiler_is_clang=yes
    else
      ac_cv_compiler_is_clang=no
    fi
  ])

  if test "x$ac_cv_compiler_is_clang" = xyes; then
    AC_PATH_PROG(LLVM_LLD, "lld")
  fi
  AC_SUBST(LLVM_LLD)
])

AX_COMPILER_IS_CLANG

if test x"$ac_cv_compiler_is_clang" = xyes; then
  USE_CLANG=yes
fi

AC_SUBST(USE_CLANG)
