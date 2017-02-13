dnl
dnl Check for pugixml
dnl

PKG_CHECK_EXISTS([pugixml],[pugixml_found=yes],[pugixml_found=no])

if test x"$pugixml_found" = xyes; then
  PKG_CHECK_MODULES([PUGIXML],[pugixml],[pugixml_found=yes],[pugixml_found=no])
fi

if test x"$pugixml_found" = xno; then
   AC_LANG_PUSH(C++)

  AC_CHECK_HEADERS([pugixml.hpp])

  if test x"$ac_cv_header_pugixml_hpp" = xyes; then
    AC_CHECK_LIB([pugixml], [main])
  fi

  AC_LANG_POP

  if test x"$ac_cv_header_pugixml_hpp" = xyes -a x"$ac_cv_lib_pugixml_main" = xyes ; then
    pugixml_found=yes
  fi
fi

if test $pugixml_found = yes; then
  AC_MSG_NOTICE([Using the system version of the pugixml library])
  PUGIXML_INTERNAL=no
else
  AC_MSG_NOTICE([Using the internal version of the pugixml library])
  PUGIXML_INTERNAL=yes
fi

AC_SUBST(PUGIXML_INTERNAL)
