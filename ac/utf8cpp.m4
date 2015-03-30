dnl
dnl Check for UTF8-CPP
dnl

AC_LANG_PUSH(C++)

AC_CHECK_HEADERS([utf8.h])

AC_LANG_POP

if test x"$ac_cv_header_utf8_h" = xyes; then
  AC_MSG_NOTICE([Using the system version of UTF8-CPP])
  UTF8CPP_INTERNAL=no
else
  AC_MSG_NOTICE([Using the internal version of UTF8-CPP])
  UTF8CPP_INTERNAL=yes
fi

AC_SUBST(UTF8CPP_INTERNAL)
