dnl
dnl Check for UTF8-CPP
dnl

AC_LANG_PUSH(C++)

AC_CHECK_HEADERS([utf8.h])

if test x"$ac_cv_header_utf8_h" = xyes; then
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <utf8.h>
#include <vector>
]],[
std::string s("ÁÂÃÄÅÆ");
std::vector<int> utf32result;
std::vector<unsigned char> utf8result;
utf8::utf8to32(s.begin(),s.end(),std::back_inserter(utf32result));
utf8::utf32to8(utf32result.begin(),utf32result.end(),std::back_inserter(utf8result));
std::string temp;
utf8::replace_invalid(s.begin(), s.end(), std::back_inserter(temp));
])],[ac_cv_utf8cpp=yes],[ac_cv_utf8cpp=no])
fi

AC_LANG_POP

if test x"$ac_cv_utf8cpp" = xyes; then
  AC_MSG_NOTICE([Using the system version of UTF8-CPP])
  UTF8CPP_INTERNAL=no
else
  AC_MSG_NOTICE([Using the internal version of UTF8-CPP])
  UTF8CPP_INTERNAL=yes
fi

AC_SUBST(UTF8CPP_INTERNAL)
