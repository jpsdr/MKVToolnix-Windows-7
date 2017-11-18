AC_CACHE_VAL([ac_cv_compiler_version_line],[ac_cv_compiler_version_line=`LC_ALL=C $CXX --version | sed -e 1q`])

# g++ (GCC) 5.3.1 20160406 (Red Hat 5.3.1-6)
# gcc (GCC) 7.2.0
# g++ (Debian 4.9.2-10) 4.9.2
# Debian clang version 3.5.0-10 (tags/RELEASE_350/final) (based on LLVM 3.5.0)
# clang version 5.0.0 (tags/RELEASE_500/final)
# Apple LLVM version 8.0.0 (clang-800.0.42.1)

case "$ac_cv_compiler_version_line" in
  *apple*|*Apple*|*clang*)
    COMPILER_TYPE=clang
    COMPILER_VERSION=`echo "$ac_cv_compiler_version_line" | sed -e 's/^[[^0-9]]*//' -e 's/[[^0-9.]].*//'`
    ;;

  *)
    COMPILER_TYPE=gcc
    COMPILER_VERSION=`echo "$ac_cv_compiler_version_line" | sed -e 's/^[[^)]]*//' -e 's/^[[^0-9]]*//' -e 's/[[^0-9.]].*//'`
    ;;
esac

AC_MSG_CHECKING([compiler type and version])
AC_MSG_RESULT([$COMPILER_TYPE $COMPILER_VERSION])

AC_SUBST(COMPILER_TYPE)
AC_SUBST(COMPILER_VERSION)
