AC_DEFUN([AX_CXX_STD_CXX_FLAG],[
  AC_CACHE_CHECK([for support for the "-std=c++17" flag], [ax_cv_std_cxx_flag],[

    AC_LANG_PUSH(C++)
    CXXFLAGS_SAVED="$CXXFLAGS"

    for flag in c++17 gnu++14 c++14 gnu++11 c++11 c++0x; do
      CXXFLAGS="$CXXFLAGS_SAVED -std=$flag"
      AC_TRY_COMPILE([], [true;], [ax_cv_std_cxx_flag="-std=$flag"], [ax_cv_std_cxx_flag="undecided"])

      if test x"$ax_cv_std_cxx_flag" != xundecided ; then
        break
      fi
    done

    if test x"$ax_cv_std_cxx_flag" = xundecided ; then
      ax_cv_std_cxx_flag="no"
    fi

    AC_LANG_POP
    CXXFLAGS="$CXXFLAGS_SAVED"
  ])

  STD_CXX=""
  if test x"$ax_cv_std_cxx_flag" != xno ; then
    STD_CXX=$ax_cv_std_cxx_flag
  fi

  AC_SUBST(STD_CXX)
])

AC_DEFUN([AX_CXX11_INITIALIZER_LISTS],[
  AC_CACHE_CHECK([for support for C++11 feature "initializer lists"], [ax_cv_cxx11_initializer_lists],[

    CXXFLAGS_SAVED=$CXXFLAGS
    CXXFLAGS="$CXXFLAGS $STD_CXX"
    export CXXFLAGS

    AC_LANG_PUSH(C++)
    AC_TRY_COMPILE(
      [
#include <string>
#include <vector>
],
      [std::vector<std::string> listy = { "asd", "qwe", "123" };],
      [ax_cv_cxx11_initializer_lists="yes"],
      [ax_cv_cxx11_initializer_lists="no"])
    AC_LANG_POP

    CXXFLAGS="$CXXFLAGS_SAVED"
  ])

  if ! test x"$ax_cv_cxx11_initializer_lists" = xyes ; then
    missing_cxx_features="$missing_cxx_features\n  * initializer lists (C++11)"
  fi
])

AC_DEFUN([AX_CXX11_RANGE_BASED_FOR],[
  AC_CACHE_CHECK([for support for C++11 feature "range-based 'for'"], [ax_cv_cxx11_range_based_for],[

    CXXFLAGS_SAVED=$CXXFLAGS
    CXXFLAGS="$CXXFLAGS $STD_CXX"
    export CXXFLAGS

    AC_LANG_PUSH(C++)
    AC_TRY_COMPILE(
      [
#include <string>
#include <vector>
],[
std::string total;
std::vector<std::string> listy = { "asd", "qwe", "123" };
for (std::string &s : listy)
  total += s;
],
      [ax_cv_cxx11_range_based_for="yes"],
      [ax_cv_cxx11_range_based_for="no"])
    AC_LANG_POP

    CXXFLAGS="$CXXFLAGS_SAVED"
  ])

  if ! test x"$ax_cv_cxx11_range_based_for" = xyes ; then
    missing_cxx_features="$missing_cxx_features\n  * range-based 'for' (C++11)"
  fi
])

AC_DEFUN([AX_CXX11_RIGHT_ANGLE_BRACKETS],[
  AC_CACHE_CHECK([for support for C++11 feature "right angle brackets"], [ax_cv_cxx11_right_angle_brackets],[

    CXXFLAGS_SAVED=$CXXFLAGS
    CXXFLAGS="$CXXFLAGS $STD_CXX"
    export CXXFLAGS

    AC_LANG_PUSH(C++)
    AC_TRY_COMPILE(
      [
#include <map>
#include <vector>

typedef std::map<int, std::vector<int>> unicorn;
],
      [unicorn charlie;],
      [ax_cv_cxx11_right_angle_brackets="yes"],
      [ax_cv_cxx11_right_angle_brackets="no"])
    AC_LANG_POP

    CXXFLAGS="$CXXFLAGS_SAVED"
  ])

  if ! test x"$ax_cv_cxx11_right_angle_brackets" = xyes ; then
    missing_cxx_features="$missing_cxx_features\n  * right angle brackets (C++11)"
  fi
])

AC_DEFUN([AX_CXX11_AUTO_KEYWORD],[
  AC_CACHE_CHECK([for support for C++11 feature "'auto' keyword"], [ax_cv_cxx11_auto_keyword],[

    CXXFLAGS_SAVED=$CXXFLAGS
    CXXFLAGS="$CXXFLAGS $STD_CXX"
    export CXXFLAGS

    AC_LANG_PUSH(C++)
    AC_TRY_COMPILE(
      [
#include <vector>
],[
  std::vector<unsigned int> listy;
  unsigned int sum = 0;
  for (auto i = listy.begin(); i < listy.end(); i++)
    sum += *i;
],
      [ax_cv_cxx11_auto_keyword="yes"],
      [ax_cv_cxx11_auto_keyword="no"])
    AC_LANG_POP

    CXXFLAGS="$CXXFLAGS_SAVED"
  ])

  if ! test x"$ax_cv_cxx11_auto_keyword" = xyes ; then
    missing_cxx_features="$missing_cxx_features\n  * 'auto' keyword (C++11)"
  fi
])

AC_DEFUN([AX_CXX11_LAMBDA_FUNCTIONS],[
  AC_CACHE_CHECK([for support for C++11 feature "lambda functions"], [ax_cv_cxx11_lambda_functions],[

    CXXFLAGS_SAVED=$CXXFLAGS
    CXXFLAGS="$CXXFLAGS $STD_CXX"
    export CXXFLAGS

    AC_LANG_PUSH(C++)
    AC_TRY_COMPILE(
      [
#include <algorithm>
#include <vector>
],
      [
std::vector<unsigned int> listy;
unsigned int sum = 0;
std::for_each(listy.begin(), listy.end(), [&](unsigned int i) { sum += i; });
],
      [ax_cv_cxx11_lambda_functions="yes"],
      [ax_cv_cxx11_lambda_functions="no"])
    AC_LANG_POP

    CXXFLAGS="$CXXFLAGS_SAVED"
  ])

  if ! test x"$ax_cv_cxx11_lambda_functions" = xyes ; then
    missing_cxx_features="$missing_cxx_features\n  * lambda functions (C++11)"
  fi
])

AC_DEFUN([AX_CXX11_NULLPTR],[
  AC_CACHE_CHECK([for support for C++11 feature "nullptr"], [ax_cv_cxx11_nullptr],[

    CXXFLAGS_SAVED=$CXXFLAGS
    CXXFLAGS="$CXXFLAGS $STD_CXX"
    export CXXFLAGS

    AC_LANG_PUSH(C++)
    AC_TRY_COMPILE(
      [],
      [unsigned char const *charlie = nullptr;],
      [ax_cv_cxx11_nullptr="yes"],
      [ax_cv_cxx11_nullptr="no"])
    AC_LANG_POP

    CXXFLAGS="$CXXFLAGS_SAVED"
  ])

  if ! test x"$ax_cv_cxx11_nullptr" = xyes ; then
    missing_cxx_features="$missing_cxx_features\n  * nullptr (C++11)"
  fi
])

AC_DEFUN([AX_CXX11_TUPLES],[
  AC_CACHE_CHECK([for support for C++11 feature "tuples"], [ax_cv_cxx11_tuples],[

    CXXFLAGS_SAVED=$CXXFLAGS
    CXXFLAGS="$CXXFLAGS $STD_CXX"
    export CXXFLAGS

    AC_LANG_PUSH(C++)
    AC_TRY_COMPILE(
      [#include <tuple>],
      [
  std::tuple<int, int, char> t = std::make_tuple(1, 2, 'c');
  std::get<2>(t) += std::get<0>(t) * std::get<1>(t);
],
      [ax_cv_cxx11_tuples="yes"],
      [ax_cv_cxx11_tuples="no"])
    AC_LANG_POP

    CXXFLAGS="$CXXFLAGS_SAVED"
  ])

  if ! test x"$ax_cv_cxx11_tuples" = xyes ; then
    missing_cxx_features="$missing_cxx_features\n  * tuples (C++11)"
  fi
])

AC_DEFUN([AX_CXX11_ALIAS_DECLARATIONS],[
  AC_CACHE_CHECK([for support for C++11 feature "alias declarations"], [ax_cv_cxx11_alias_declarations],[

    CXXFLAGS_SAVED=$CXXFLAGS
    CXXFLAGS="$CXXFLAGS $STD_CXX"
    export CXXFLAGS

    AC_LANG_PUSH(C++)
    AC_TRY_COMPILE(
      [
#include <vector>
using thingy = std::vector<int>;
],[
  thingy things;
  things.push_back(42);
],
      [ax_cv_cxx11_alias_declarations="yes"],
      [ax_cv_cxx11_alias_declarations="no"])
    AC_LANG_POP

    CXXFLAGS="$CXXFLAGS_SAVED"
  ])

  if ! test x"$ax_cv_cxx11_alias_declarations" = xyes ; then
    missing_cxx_features="$missing_cxx_features\n  * alias declarations (C++11)"
  fi
])

AC_DEFUN([AX_CXX14_MAKE_UNIQUE],[
  AC_CACHE_CHECK([for support for C++14 feature "std::make_unique"], [ax_cv_cxx14_make_unique],[

    CXXFLAGS_SAVED=$CXXFLAGS
    CXXFLAGS="$CXXFLAGS $STD_CXX"
    export CXXFLAGS

    AC_LANG_PUSH(C++)
    AC_TRY_COMPILE(
      [#include <memory>],
      [auto i_ptr{std::make_unique<int>(42)};],
      [ax_cv_cxx14_make_unique="yes"],
      [ax_cv_cxx14_make_unique="no"])
    AC_LANG_POP

    CXXFLAGS="$CXXFLAGS_SAVED"
  ])

  if ! test x"$ax_cv_cxx14_make_unique" = xyes ; then
    missing_cxx_features="$missing_cxx_features\n  * std::make_unique function (C++14)"
  fi
])

AC_DEFUN([AX_CXX14_DIGIT_SEPARATORS],[
  AC_CACHE_CHECK([for support for C++14 feature "digit separators"], [ax_cv_cxx14_digit_separators],[

    CXXFLAGS_SAVED=$CXXFLAGS
    CXXFLAGS="$CXXFLAGS $STD_CXX"
    export CXXFLAGS

    AC_LANG_PUSH(C++)
    AC_TRY_COMPILE(
      [],
      [auto num = 10'000'000'000ll;],
      [ax_cv_cxx14_digit_separators="yes"],
      [ax_cv_cxx14_digit_separators="no"])
    AC_LANG_POP

    CXXFLAGS="$CXXFLAGS_SAVED"
  ])

  if ! test x"$ax_cv_cxx14_digit_separators" = xyes ; then
    missing_cxx_features="$missing_cxx_features\n  * digit separators (C++14)"
  fi
])

AC_DEFUN([AX_CXX14_BINARY_LITERALS],[
  AC_CACHE_CHECK([for support for C++14 feature "binary literals"], [ax_cv_cxx14_binary_literals],[

    CXXFLAGS_SAVED=$CXXFLAGS
    CXXFLAGS="$CXXFLAGS $STD_CXX"
    export CXXFLAGS

    AC_LANG_PUSH(C++)
    AC_TRY_COMPILE(
      [],
      [auto num1 = 0b00101010;
       auto num2 = 0B00101010;
       auto num3 = 0b0010'1010;],
      [ax_cv_cxx14_binary_literals="yes"],
      [ax_cv_cxx14_binary_literals="no"])
    AC_LANG_POP

    CXXFLAGS="$CXXFLAGS_SAVED"
  ])

  if ! test x"$ax_cv_cxx14_binary_literals" = xyes ; then
    missing_cxx_features="$missing_cxx_features\n  * binary literals (C++14)"
  fi
])

AC_DEFUN([AX_CXX14_GENERIC_LAMBDAS],[
  AC_CACHE_CHECK([for support for C++14 feature "generic lambdas"], [ax_cv_cxx14_generic_lambdas],[

    CXXFLAGS_SAVED=$CXXFLAGS
    CXXFLAGS="$CXXFLAGS $STD_CXX"
    export CXXFLAGS

    AC_LANG_PUSH(C++)
    AC_TRY_COMPILE(
      [
        #include <algorithm>
        #include <vector>
      ],
      [
        std::vector<int> vv;
        std::sort(vv.begin(), vv.end(), [](auto &a, auto &b) { return b < a; });
      ],
      [ax_cv_cxx14_generic_lambdas="yes"],
      [ax_cv_cxx14_generic_lambdas="no"])
    AC_LANG_POP

    CXXFLAGS="$CXXFLAGS_SAVED"
  ])

  if ! test x"$ax_cv_cxx14_generic_lambdas" = xyes ; then
    missing_cxx_features="$missing_cxx_features\n  * generic lambdas (C++14)"
  fi
])

AC_DEFUN([AX_CXX14_USER_DEFINED_LITERALS_FOR_STD_STRING],[
  AC_CACHE_CHECK([for support for C++14 feature "User-defined literals for std::string"], [ax_cv_cxx14_user_defined_literals_for_std_string],[

    CXXFLAGS_SAVED=$CXXFLAGS
    CXXFLAGS="$CXXFLAGS $STD_CXX"
    export CXXFLAGS

    AC_LANG_PUSH(C++)
    AC_TRY_COMPILE(
      [
#include <string>
using namespace std::string_literals;
      ],
      [return "hello"s.length();],
      [ax_cv_cxx14_user_defined_literals_for_std_string="yes"],
      [ax_cv_cxx14_user_defined_literals_for_std_string="no"])
    AC_LANG_POP

    CXXFLAGS="$CXXFLAGS_SAVED"
  ])

  if ! test x"$ax_cv_cxx14_user_defined_literals_for_std_string" = xyes ; then
    missing_cxx_features="$missing_cxx_features\n  * User-defined literals for std::string (C++14)"
  fi
])

AC_DEFUN([AX_CXX17_ATTRIBUTE_MAYBE_UNUSED],[
  AC_CACHE_CHECK([for support for C++17 feature "attribute 'maybe_unused'"], [ax_cv_cxx17_attribute_maybe_unused],[

    CXXFLAGS_SAVED=$CXXFLAGS
    CXXFLAGS="$CXXFLAGS $STD_CXX"
    export CXXFLAGS

    AC_LANG_PUSH(C++)
    AC_TRY_COMPILE(
      [
int testme([[maybe_unused]] int the_var) {
  return 42;
}
      ],
      [return testme(54);],
      [ax_cv_cxx17_attribute_maybe_unused="yes"],
      [ax_cv_cxx17_attribute_maybe_unused="no"])
    AC_LANG_POP

    CXXFLAGS="$CXXFLAGS_SAVED"
  ])

  if ! test x"$ax_cv_cxx17_attribute_maybe_unused" = xyes ; then
    missing_cxx_features="$missing_cxx_features\n  * attribute 'maybe_unused' (C++17)"
  fi
])

AC_DEFUN([AX_CXX17_STRUCTURED_BINDINGS],[
  AC_CACHE_CHECK([for support for C++17 feature "structured bindings"], [ax_cv_cxx17_structured_bindings],[

    CXXFLAGS_SAVED=$CXXFLAGS
    CXXFLAGS="$CXXFLAGS $STD_CXX"
    export CXXFLAGS

    AC_LANG_PUSH(C++)
    AC_TRY_COMPILE(
      [
#include <utility>
std::pair<int, char> testme() {
  return std::make_pair(42, '!');
}
      ],
      [
auto const &[the_int, the_char] = testme();
return the_int;
      ],
      [ax_cv_cxx17_structured_bindings="yes"],
      [ax_cv_cxx17_structured_bindings="no"])
    AC_LANG_POP

    CXXFLAGS="$CXXFLAGS_SAVED"
  ])

  if ! test x"$ax_cv_cxx17_structured_bindings" = xyes ; then
    missing_cxx_features="$missing_cxx_features\n  * structured bindings (C++17)"
  fi
])

AC_DEFUN([AX_CXX17_NESTED_NAMESPACE_DEFINITION],[
  AC_CACHE_CHECK([for support for C++17 feature "nested namespace definition"], [ax_cv_cxx17_nested_namespace_definition],[

    CXXFLAGS_SAVED=$CXXFLAGS
    CXXFLAGS="$CXXFLAGS $STD_CXX"
    export CXXFLAGS

    AC_LANG_PUSH(C++)
    AC_TRY_COMPILE(
      [namespace A::B::C { int d; }],
      [A::B::C::d = 1; return A::B::C::d;],
      [ax_cv_cxx17_nested_namespace_definition="yes"],
      [ax_cv_cxx17_nested_namespace_definition="no"])
    AC_LANG_POP

    CXXFLAGS="$CXXFLAGS_SAVED"
  ])

  if ! test x"$ax_cv_cxx17_nested_namespace_definition" = xyes ; then
    missing_cxx_features="$missing_cxx_features\n  * nested namespace definition (C++17)"
  fi
])

AC_DEFUN([AX_CXX17_STD_OPTIONAL],[
  AC_CACHE_CHECK([for support for C++17 feature "std::optional"], [ax_cv_cxx17_std_optional],[

    CXXFLAGS_SAVED=$CXXFLAGS
    CXXFLAGS="$CXXFLAGS $STD_CXX"
    export CXXFLAGS

    AC_LANG_PUSH(C++)
    AC_TRY_COMPILE(
      [#include <optional>],
      [
  std::optional<int> moo;
  moo = 42;
  return moo ? moo.value() : moo.value_or(54);
],
      [ax_cv_cxx17_std_optional="yes"],
      [ax_cv_cxx17_std_optional="no"])
    AC_LANG_POP

    CXXFLAGS="$CXXFLAGS_SAVED"
  ])

  if ! test x"$ax_cv_cxx17_std_optional" = xyes ; then
    missing_cxx_features="$missing_cxx_features\n  * std::optional (C++17)"
  fi
])

AC_DEFUN([AX_CXX17_DEF_NAME],[
dnl   AC_CACHE_CHECK([for support for C++17 feature "human"], [ax_cv_cxx17_def_name],[
dnl
dnl     CXXFLAGS_SAVED=$CXXFLAGS
dnl     CXXFLAGS="$CXXFLAGS $STD_CXX"
dnl     export CXXFLAGS
dnl
dnl     AC_LANG_PUSH(C++)
dnl     AC_TRY_COMPILE(
dnl       [],
dnl       [],
dnl       [ax_cv_cxx17_def_name="yes"],
dnl       [ax_cv_cxx17_def_name="no"])
dnl     AC_LANG_POP
dnl
dnl     CXXFLAGS="$CXXFLAGS_SAVED"
dnl   ])
dnl
dnl   if ! test x"$ax_cv_cxx17_def_name" = xyes ; then
dnl     missing_cxx_features="$missing_cxx_features\n  * human (C++17)"
dnl   fi
dnl ])

AX_CXX_STD_CXX_FLAG
AX_CXX11_INITIALIZER_LISTS
AX_CXX11_RANGE_BASED_FOR
AX_CXX11_RIGHT_ANGLE_BRACKETS
AX_CXX11_AUTO_KEYWORD
AX_CXX11_LAMBDA_FUNCTIONS
AX_CXX11_NULLPTR
AX_CXX11_TUPLES
AX_CXX11_ALIAS_DECLARATIONS
AX_CXX14_MAKE_UNIQUE
AX_CXX14_DIGIT_SEPARATORS
AX_CXX14_BINARY_LITERALS
AX_CXX14_GENERIC_LAMBDAS
AX_CXX14_USER_DEFINED_LITERALS_FOR_STD_STRING
AX_CXX17_ATTRIBUTE_MAYBE_UNUSED
AX_CXX17_NESTED_NAMESPACE_DEFINITION
AX_CXX17_STRUCTURED_BINDINGS
AX_CXX17_STD_OPTIONAL

if test x"$missing_cxx_features" != x ; then
  printf "The following features of the C++11/C++14/C++17 standards are not supported by $CXX:$missing_cxx_features\n"
  printf "If you are using the GNU C compiler collection (gcc), you need\n"
  printf "at least v7; for clang v4 and newer should work.\n"
  AC_MSG_ERROR([support for required C++11/C++14/C++17 features incomplete])
fi
