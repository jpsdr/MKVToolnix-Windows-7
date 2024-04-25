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

    #if !defined(FMT_VERSION) || (FMT_VERSION < 80000)
    #error fmtlib is too old, need 8.0.0 or later
    #endif
  ]], [[
    fmt::format("{0}", fmt::to_string(4254));
    std::string moo{"'{0}' says the cow"};
    fmt::format(fmt::runtime(moo), "moo");
  ]])],[ac_cv_fmt=yes],[ac_cv_fmt=no])

  CXXFLAGS="$ac_save_CXXFLAGS"
  LIBS="$ac_save_LIBS"

  AC_LANG_POP
])

if test x"$ac_cv_fmt" = xyes; then
  FMT_INTERNAL=no
else
  AC_MSG_NOTICE([Using the internal version of fmt])
  FMT_INTERNAL=yes
fi

AC_SUBST(FMT_INTERNAL)
