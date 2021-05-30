# boost's headers must be present.
AX_BOOST_BASE([1.66.0])

AX_BOOST_CHECK_HEADERS([boost/multiprecision/cpp_int.hpp],,[
  AC_MSG_ERROR([Boost's multi-precision library is required but wasn't found])
])

AX_BOOST_CHECK_HEADERS([boost/operators.hpp],,[
  AC_MSG_ERROR([Boost's Operators library is required but wasn't found])
])

AX_BOOST_CHECK_HEADERS([boost/rational.hpp],,[
  AC_MSG_ERROR([Boost's rational library is required but wasn't found])
])
