dnl
dnl Debugging
dnl

AC_ARG_ENABLE([debug],
  AS_HELP_STRING([--enable-debug],[compile with debug information (no)]),
  [],
  [enable_debug=no])

if test x"$enable_debug" = xyes ; then
  opt_features_yes="$opt_features_yes\n   * debugging information"
else
  opt_features_no="$opt_features_no\n   * debugging information"
fi

USE_DEBUG=$enable_debug
AC_SUBST(USE_DEBUG)


dnl
dnl Profiling
dnl

AC_ARG_ENABLE([profiling],
  AS_HELP_STRING([--enable-profiling],[compile with profiling information (no)]),
  [],
  [enable_profiling=no])

if test x"$enable_profiling" = xyes ; then
  opt_features_yes="$opt_features_yes\n   * profiling support"
else
  opt_features_no="$opt_features_no\n   * profiling support"
fi

USE_PROFILING=$enable_profiling
AC_SUBST(USE_PROFILING)


dnl
dnl Optimization
dnl

AC_ARG_ENABLE([optimization],
  AS_HELP_STRING([--enable-optimization],[compile with optimization: -O3 (yes)]),
  [],
  [if test x"$enable_debug" = xyes ; then
     enable_optimization=no
   else
     enable_optimization=yes
   fi])

if test x"$enable_optimization" = xyes; then
  if test $COMPILER_TYPE = clang && ! check_version 3.8.0 $COMPILER_VERSION; then
    opt_features_yes="$opt_features_yes\n   * partial optimizations: due to bug 11962 in LLVM/clang only -O1 will be used for optimization"

  elif test "x$ac_cv_mingw32" = "xyes" -a "x$MINGW_PROCESSOR_ARCH" = "xx86" && check_version 5.1.0 $COMPILER_VERSION && ! check_version 7.2.0 $COMPILER_VERSION; then
    opt_features_yes="$opt_features_yes\n   * partial optimizations: due to an issue in mingw g++ >= 5.1.0 and < 7.2.0 full optimization cannot be used"

  else
    opt_features_yes="$opt_features_yes\n   * compiler optimizations"

  fi
else
  opt_features_no="$opt_features_no\n   * compiler optimizations"
fi

USE_OPTIMIZATION=$enable_optimization
AC_SUBST(USE_OPTIMIZATION)


dnl
dnl Address sanitizer
dnl

AC_ARG_ENABLE([addrsan],
  AS_HELP_STRING([--enable-addrsan],[compile with address sanitization turned on (no)]),
  [],[enable_addrsan=no])

if test x"$enable_addrsan" = xyes ; then
  opt_features_yes="$opt_features_yes\n   * development technique 'address sanitizer'"
else
  opt_features_no="$opt_features_no\n   * development technique 'address sanitizer'"
fi

USE_ADDRSAN=$enable_addrsan
AC_SUBST(USE_ADDRSAN)


dnl
dnl Undefined behavior sanitizer
dnl

AC_ARG_ENABLE([ubsan],
  AS_HELP_STRING([--enable-ubsan],[compile with sanitization for undefined behavior turned on (no)]),
  [],[enable_ubsan=no])

if test x"$enable_ubsan" = xyes ; then
  opt_features_yes="$opt_features_yes\n   * development technique 'undefined behavior sanitizer'"
else
  opt_features_no="$opt_features_no\n   * development technique 'undefined behavior sanitizer'"
fi

USE_UBSAN=$enable_ubsan
AC_SUBST(USE_UBSAN)
