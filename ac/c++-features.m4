AC_DEFUN([AX_CXX_STD_CXX_FLAG],[
  AC_CACHE_CHECK([for support for the "-std=c++20" or "-std=c++17" flag], [ax_cv_std_cxx_flag],[

    AC_LANG_PUSH(C++)
    CXXFLAGS_SAVED="$CXXFLAGS"

    for flag in c++20 c++17; do
      CXXFLAGS="$CXXFLAGS_SAVED -std=$flag"
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[true;]])],[ax_cv_std_cxx_flag="-std=$flag"],[ax_cv_std_cxx_flag="undecided"])

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

AC_DEFUN([AX_CXX17_ATTRIBUTE_MAYBE_UNUSED],[
  AC_CACHE_CHECK([for support for C++17 feature "attribute 'maybe_unused'"], [ax_cv_cxx17_attribute_maybe_unused],[

    CXXFLAGS_SAVED=$CXXFLAGS
    CXXFLAGS="$CXXFLAGS $STD_CXX"
    export CXXFLAGS

    AC_LANG_PUSH(C++)
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
int testme([[maybe_unused]] int the_var) {
  return 42;
}
      ]], [[return testme(54);]])],
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
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <utility>
std::pair<int, char> testme() {
  return std::make_pair(42, '!');
}
      ]], [[
auto const &[the_int, the_char] = testme();
return the_int;
      ]])],
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
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
        [[namespace A::B::C { int d; }]],
        [[A::B::C::d = 1; return A::B::C::d;]])],
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
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
        [[#include <optional>]], [[
          std::optional<int> moo;
          moo = 42;
          return moo ? moo.value() : moo.value_or(54);
        ]])],
      [ax_cv_cxx17_std_optional="yes"],
      [ax_cv_cxx17_std_optional="no"])
    AC_LANG_POP

    CXXFLAGS="$CXXFLAGS_SAVED"
  ])

  if ! test x"$ax_cv_cxx17_std_optional" = xyes ; then
    missing_cxx_features="$missing_cxx_features\n  * std::optional (C++17)"
  fi
])

AC_DEFUN([AX_CXX17_STD_GCD],[
  AC_CACHE_CHECK([for support for C++17 feature "std::gcd"], [ax_cv_cxx17_std_gcd],[

    CXXFLAGS_SAVED=$CXXFLAGS
    CXXFLAGS="$CXXFLAGS $STD_CXX"
    export CXXFLAGS

    AC_LANG_PUSH(C++)
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
        [[#include <numeric>]],
        [[return std::gcd(42, 54);]])],
      [ax_cv_cxx17_std_gcd="yes"],
      [ax_cv_cxx17_std_gcd="no"])
    AC_LANG_POP

    CXXFLAGS="$CXXFLAGS_SAVED"
  ])

  if ! test x"$ax_cv_cxx17_std_gcd" = xyes ; then
    missing_cxx_features="$missing_cxx_features\n  * std::gcd (C++17)"
  fi
])

AC_DEFUN([AX_CXX17_CONSTEXPR_IF],[
  AC_CACHE_CHECK([for support for C++17 feature "constexpr if"], [ax_cv_cxx17_constexpr_if],[

    CXXFLAGS_SAVED=$CXXFLAGS
    CXXFLAGS="$CXXFLAGS $STD_CXX"
    export CXXFLAGS

    AC_LANG_PUSH(C++)
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[if constexpr (true) return 42; else return 54;]])],
      [ax_cv_cxx17_constexpr_if="yes"],
      [ax_cv_cxx17_constexpr_if="no"])
    AC_LANG_POP

    CXXFLAGS="$CXXFLAGS_SAVED"
  ])

  if ! test x"$ax_cv_cxx17_constexpr_if" = xyes ; then
    missing_cxx_features="$missing_cxx_features\n  * constexpr if (C++17)"
  fi
])

AC_DEFUN([AX_CXX17_LIBSTDCPPFS],[
  AC_CACHE_CHECK([for libraries to link against for the file system library], [ax_cv_cxx17_libstdcppfs],[

    CXXFLAGS_SAVED=$CXXFLAGS
    LIBS_SAVED="$LIBS"
    CXXFLAGS="$CXXFLAGS $STD_CXX"
    LIBS="$LIBS -lstdc++fs"
    export CXXFLAGS LIBS

    AC_LANG_PUSH(C++)
    AC_LINK_IFELSE([AC_LANG_PROGRAM(
        [[#include <filesystem>]],
        [[return std::filesystem::exists(std::filesystem::path{"/etc/passwd"}) ? 1 : 0;]])],
      [ax_cv_cxx17_libstdcppfs="-lstdc++fs"],
      [ax_cv_cxx17_libstdcppfs=""])
    AC_LANG_POP

    CXXFLAGS="$CXXFLAGS_SAVED"
    LIBS="$LIBS_SAVED"
  ])

  STDCPPFS_LIBS="$ax_cv_cxx17_libstdcppfs"

  AC_SUBST(STDCPPFS_LIBS)
])

dnl AC_DEFUN([AX_CXX17_DEF_NAME],[
dnl   AC_CACHE_CHECK([for support for C++17 feature "human"], [ax_cv_cxx17_def_name],[
dnl
dnl     CXXFLAGS_SAVED=$CXXFLAGS
dnl     CXXFLAGS="$CXXFLAGS $STD_CXX"
dnl     export CXXFLAGS
dnl
dnl     AC_LANG_PUSH(C++)
dnl     AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
dnl         [[#include <…>]],
dnl         [[code("…");]])],
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
AX_CXX17_ATTRIBUTE_MAYBE_UNUSED
AX_CXX17_NESTED_NAMESPACE_DEFINITION
AX_CXX17_STRUCTURED_BINDINGS
AX_CXX17_STD_OPTIONAL
AX_CXX17_STD_GCD
AX_CXX17_CONSTEXPR_IF
AX_CXX17_LIBSTDCPPFS

if test x"$missing_cxx_features" != x ; then
  printf "The following features of the C++17 standards are not supported by $CXX:$missing_cxx_features\n"
  printf "If you are using the GNU C compiler collection (gcc), you need\n"
  printf "at least v8; for clang v7 and newer should work.\n"
  AC_MSG_ERROR([support for required C++17 features incomplete])
fi
