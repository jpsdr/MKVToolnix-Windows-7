dnl
dnl Check for nlohmann's json-cpp library
dnl


AC_CACHE_CHECK([fmt],[ac_cv_fmt],[
  AC_LANG_PUSH(C++)

  ac_save_CXXFLAGS="$CXXFLAGS"
  ac_save_LIBS="$LIBS"
  CXXFLAGS="$STD_CXX $CXXFLAGS"
  LIBS="$LIBS -lfmt"

  AC_TRY_COMPILE([
    #include <fmt/format.h>
    #include <fmt/ostream.h>
  ],[
    fmt::format("{0:02}", fmt::to_string(4254));
  ],[ac_cv_fmt=yes],[ac_cv_fmt=no])

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
