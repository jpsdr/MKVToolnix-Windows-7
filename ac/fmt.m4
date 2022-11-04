dnl
dnl Check for the fmt library
dnl


AC_CACHE_CHECK([fmt],[ac_cv_fmt],[
  AC_LANG_PUSH(C++)

  ac_save_CXXFLAGS="$CXXFLAGS"
  ac_save_LIBS="$LIBS"
  CXXFLAGS="$STD_CXX $CXXFLAGS"
  LIBS="$LIBS -lfmt"

  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
    #include <fmt/format.h>
    #include <fmt/ostream.h>

    #if !defined(FMT_VERSION) || (FMT_VERSION < 60100)
    #error fmtlib is too old, need 6.1.0 or later
    #endif
  ]], [[
    fmt::format("{0:02}", fmt::to_string(4254));
  ]])],[ac_cv_fmt=yes],[ac_cv_fmt=no])

  AC_CACHE_VAL(ac_cv_fmt_v8, [
    if test $ac_cv_fmt=yes; then
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
        #include <fmt/format.h>
        #include <fmt/ostream.h>

        #if !defined(FMT_VERSION) || (FMT_VERSION < 80000)
        #error fmt is older than v8
        #endif
      ]], [[
        fmt::format("{0:02}", fmt::to_string(4254));
      ]])],[ac_cv_fmt_v8=yes],[ac_cv_fmt_v8=no])
    fi
  ])

  CXXFLAGS="$ac_save_CXXFLAGS"
  LIBS="$ac_save_LIBS"

  AC_LANG_POP
])

if test x"$ac_cv_fmt" = xyes; then
  FMT_INTERNAL=no
else
  AC_MSG_NOTICE([Using the internal version of fmt])
  FMT_INTERNAL=yes
  ac_cv_fmt_v8=yes
fi

if test $ac_cv_fmt_v8 = yes; then
  AC_DEFINE([HAVE_FMT_V8],[1],[Define if the fmt library is v8 or newer])
fi
AC_SUBST(FMT_INTERNAL)
