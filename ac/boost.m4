# boost's headers must be present.
AX_BOOST_BASE([1.74.0])

AC_MSG_CHECKING([for Boost's multi-precision library with GMP backend])

saved_CPPFLAGS="$CPPFLAGS"
saved_LDFLAGS="$LDFLAGS"
saved_LIBS="$LIBS"

CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
export CPPFLAGS
LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
export LDFLAGS
LIBS="$LIBS -lgmp"
export LIBS

AC_LANG_PUSH(C++)

AC_COMPILE_IFELSE([
  AC_LANG_PROGRAM([[
    #include <boost/multiprecision/gmp.hpp>
  ]],
  [[
    boost::multiprecision::mpz_int i{42};
    boost::multiprecision::mpq_rational r{42, 54};
  ]])
],[am_cv_bmp_gmp=yes],[am_cv_bmp_gmp=no])

CPPFLAGS="$saved_CPPFLAGS"
LDFLAGS="$saved_LDFLAGS"
LIBS="$saved_LIBS"

if test "x$am_cv_bmp_gmp" != xyes; then
  AC_MSG_RESULT([no: a test program for Boost's multi-precision library with GMP backend could not be linked, probably because the gmp library wasn't found])
  exit 1
fi
AC_MSG_RESULT([yes])

# boost::system must be present.
AX_BOOST_CHECK_HEADERS([boost/system/error_code.hpp],,[
  AC_MSG_ERROR([Boost's system library is required but wasn't found])
])

# boost::filesystem must be present.
AX_BOOST_FILESYSTEM()

if test x"$ax_cv_boost_filesystem" != "xyes"; then
  AC_MSG_ERROR(The Boost Filesystem Library was not found.)
fi

AX_BOOST_CHECK_HEADERS([boost/operators.hpp],,[
  AC_MSG_ERROR([Boost's Operators library is required but wasn't found])
])
