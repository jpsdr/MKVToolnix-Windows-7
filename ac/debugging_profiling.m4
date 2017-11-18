dnl
dnl Debugging, profiling and optimization options
dnl

AC_ARG_ENABLE([debug],
  AC_HELP_STRING([--enable-debug],[compile with debug information (no)]),
  [],
  [enable_debug=no])

AC_ARG_ENABLE([profiling],
  AC_HELP_STRING([--enable-profiling],[compile with profiling information (no)]),
  [],
  [enable_profiling=no])

AC_ARG_ENABLE([optimization],
  AC_HELP_STRING([--enable-optimization],[compile with optimization: -O3 (yes)]),
  [],
  [if test x"$enable_debug" = xyes ; then
     enable_optimization=no
   else
     enable_optimization=yes
   fi])

DEBUG_CFLAGS=""
OPTIMIZATION_CFLAGS=""
PROFILING_CFLAGS=""
PROFILING_LIBS=""

if test x"$enable_debug" = xyes ; then
  DEBUG_CFLAGS="-g -DDEBUG"
  opt_features_yes="$opt_features_yes\n   * debugging information"
else
  opt_features_no="$opt_features_no\n   * debugging information"
fi

if test x"$enable_optimization" = xyes; then
  if test $COMPILER_TYPE = clang && ! check_version 3.8.0 $COMPILER_VERSION; then
    opt_features_no="$opt_features_no\n   * full optimization: due to bug 11962 in LLVM/clang only -O1 will be used for optimization"
    OPTIMIZATION_CFLAGS="-O1"

  elif test "x$ac_cv_mingw32" = "xyes" -a "x$MINGW_PROCESSOR_ARCH" = "xx86" && check_version 5.1.0 $COMPILER_VERSION && ! check_version 7.2.0 $COMPILER_VERSION; then
    OPTIMIZATION_CFLAGS="-O2 -fno-ipa-icf"
    opt_features_no="$opt_features_no\n   * full optimization: due to an issue in mingw g++ >= 5.1.0 and < 7.2.0 full optimization cannot be used"

  else
    OPTIMIZATION_CFLAGS="-O3"
  fi

  opt_features_yes="$opt_features_yes\n   * compiler optimizations ($OPTIMIZATION_CFLAGS)"
else
  opt_features_no="$opt_features_no\n   * compiler optimizations"
fi

if test x"$enable_profiling" = xyes ; then
  PROFILING_CFLAGS="-pg"
  PROFILING_LIBS="-pg"
  opt_features_yes="$opt_features_yes\n   * profiling support"
else
  opt_features_no="$opt_features_no\n   * profiling support"
fi

AC_ARG_ENABLE([addrsan],
  AC_HELP_STRING([--enable-addrsan],[compile with address sanitization turned on (no)]),
  [ADDRSAN=yes],[ADDRSAN=no])

if test x"$ADDRSAN" = xyes ; then
  opt_features_yes="$opt_features_yes\n   * development technique 'address sanitizer'"
else
  opt_features_no="$opt_features_no\n   * development technique 'address sanitizer'"
fi

AC_ARG_ENABLE([ubsan],
  AC_HELP_STRING([--enable-ubsan],[compile with sanitization for undefined behavior turned on (no)]),
  [UBSAN=yes],[UBSAN=no])

if test x"$UBSAN" = xyes ; then
  opt_features_yes="$opt_features_yes\n   * development technique 'undefined behavior sanitizer'"
else
  opt_features_no="$opt_features_no\n   * development technique 'undefined behavior sanitizer'"
fi

AC_SUBST(DEBUG_CFLAGS)
AC_SUBST(PROFILING_CFLAGS)
AC_SUBST(PROFILING_LIBS)
AC_SUBST(OPTIMIZATION_CFLAGS)
AC_SUBST(ADDRSAN)
AC_SUBST(UBSAN)
