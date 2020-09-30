dnl
dnl Check for pcre2
dnl

PKG_CHECK_EXISTS([libpcre2-8],[libpcre2_8_found=yes],[libpcre2_8_found=no])
if test x"$libpcre2_8_found" = xyes; then
  PKG_CHECK_MODULES([PCRE2],[libpcre2-8],[libpcre2_8_found=yes])
  PCRE2_CFLAGS="`$PKG_CONFIG --cflags libpcre2-8`"
  PCRE2_LIBS="`$PKG_CONFIG --libs libpcre2-8`"
fi

if test x"$libpcre2_8_found" != xyes; then
  AC_MSG_ERROR([Could not find the PCRE2 library])
fi

AC_SUBST(PCRE2_CFLAGS)
AC_SUBST(PCRE2_LIBS)

dnl
dnl Check for jpcre2
dnl

AC_CACHE_CHECK([for JPCRE2],[ac_cv_jpcre2],[
  AC_LANG_PUSH(C++)

  ac_save_CXXFLAGS="$CXXFLAGS"
  ac_save_LIBS="$LIBS"
  CXXFLAGS="$STD_CXX $CXXFLAGS"
  LIBS="$LIBS -ljpcre2"

  AC_TRY_COMPILE([
    #include <jpcre2.hpp>

    #if !defined(JPCRE2_VERSION) || (JPCRE2_VERSION < 103201)
    #error jpcre2 is too old, need 10.32.1 or later
    #endif
  ],[
    using jp = jpcre2::select<char>;
    jp::Regex regex{"moo"};
    jp::RegexMatch()
      .setRegexObject(&regex)
      .setSubject("cow")
      .match();
  ],[ac_cv_jpcre2=yes],[ac_cv_jpcre2=no])

  CXXFLAGS="$ac_save_CXXFLAGS"
  LIBS="$ac_save_LIBS"

  AC_LANG_POP
])

if test x"$ac_cv_jpcre2" = xyes; then
  JPCRE2_INTERNAL=no
else
  AC_MSG_NOTICE([Using the internal version of jpcre2])
  JPCRE2_INTERNAL=yes
fi

AC_SUBST(JPCRE2_INTERNAL)
