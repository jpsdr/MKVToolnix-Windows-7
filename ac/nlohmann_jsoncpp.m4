dnl
dnl Check for nlohmann's json-cpp library
dnl


AC_CACHE_CHECK([nlohmann's json-cpp],[ac_cv_nlohmann_jsoncpp],[
  AC_LANG_PUSH(C++)

  ac_save_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$STD_CXX $CXXFLAGS"

  AC_TRY_COMPILE([
    #include <cstdint>
    #include <iostream>
    #include <limits>

    #include <json.hpp>
  ],[
    nlohmann::json json{
      { "unsigned_64bit_integer", std::numeric_limits<std::uint64_t>::max() },
      { "signed_64bit_integer",   std::numeric_limits<std::int64_t>::min()  },
    };

    std::cout << json.dump();
  ],[ac_cv_nlohmann_jsoncpp=yes],[ac_cv_nlohmann_jsoncpp=no])

  CXXFLAGS="$ac_save_CXXFLAGS"

  AC_LANG_POP
])

if test x"$ac_cv_nlohmann_jsoncpp" = xyes; then
  AC_MSG_NOTICE([Using the system version of nlohmann json-cpp])
  AC_DEFINE([HAVE_NLOHMANN_JSONCPP],[1],[Define if nlohmann's json-cpp is available.])
else
  AC_MSG_NOTICE([Using the internal version of nlohmann json-cpp])
fi
