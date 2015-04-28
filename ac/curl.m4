dnl
dnl Check for libcurl
dnl

curl_found=no
AC_ARG_WITH([curl], AC_HELP_STRING([--without-curl], [do not build with CURL support]),
            [ with_curl=${withval} ], [ with_curl=yes ])
AC_ARG_WITH(curl_config,
  AC_HELP_STRING([--with-curl-config=prog],[use prog instead of looking for curl-config]),
  [ CURL_CONFIG="$with_curl_config" ],)

if test "x$with_curl" != "xno"; then
  AC_PATH_PROG(CURL_CONFIG, curl-config, no)

  if test x"$CURL_CONFIG" != "xno" ; then
    curl_found=yes
    CURL_CFLAGS="`"$CURL_CONFIG" --cflags`"
    CURL_LIBS="`"$CURL_CONFIG" --libs`"

  else
    PKG_CHECK_MODULES([CURL], [libcurl], [curl_found=yes])
  fi
fi

if test "$curl_found" = "yes"; then
  opt_features_yes="$opt_features_yes\n   * online update checks (via libcurl)"
  AC_DEFINE(HAVE_CURL_EASY_H, 1, [define if libcurl is found via pkg-config])
else
  opt_features_no="$opt_features_no\n   * online update checks (via libcurl)"
  CURL_CFLAGS=""
  CURL_LIBS=""
fi
