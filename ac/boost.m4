# boost's headers must be present.
AX_BOOST_BASE([1.60.0])

AX_BOOST_CHECK_HEADERS([boost/rational.hpp],,[
  AC_MSG_ERROR([Boost's rational library is required but wasn't found])
])

AX_BOOST_CHECK_HEADERS([boost/operators.hpp],,[
  AC_MSG_ERROR([Boost's Operators library is required but wasn't found])
])
