AC_CACHE_CHECK(
  [compiler version],
  [ac_cv_compiler_version],
  [
  if test x"$ac_cv_compiler_is_clang" = xyes; then
    ac_cv_compiler_version=`LC_ALL=C $CXX --version | sed -e 's/^[[^0-9]]*//' -e 's/[[^0-9.]].*//' -e '1q'`
  else
    ac_cv_compiler_version=`$CXX -dumpversion`
  fi
])
