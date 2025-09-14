AC_DEFUN([AX_CXX_STD_CXX_FLAG],[
  AC_CACHE_CHECK([for support for the "-std=c++20" flag], [ax_cv_std_cxx_flag],[

    AC_LANG_PUSH(C++)
    CXXFLAGS_SAVED="$CXXFLAGS"

    for flag in c++20; do
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
  else
    AC_MSG_ERROR([compiler does not support the -std=c++20 flag])
  fi

  AC_SUBST(STD_CXX)
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

AC_DEFUN([AX_CXX20_BIT_CAST],[
  AC_CACHE_CHECK([for std::bit_cast], [ax_cv_cxx20_bit_cast],[

    CXXFLAGS_SAVED=$CXXFLAGS
    CXXFLAGS="$CXXFLAGS $STD_CXX"
    export CXXFLAGS

    AC_LANG_PUSH(C++)
    AC_LINK_IFELSE([AC_LANG_PROGRAM(
        [[#include <bit>]],
        [[unsigned int v1 = 42;
          return std::bit_cast<>(v1);]])],
      [ax_cv_cxx20_bit_cast="yes"],
      [ax_cv_cxx20_bit_cast="no"])
    AC_LANG_POP

    CXXFLAGS="$CXXFLAGS_SAVED"
  ])

  if test x"$ax_cv_cxx20_bit_cast" == xyes; then
    AC_DEFINE(HAVE_STD_BIT_CAST, 1, [Define if std::bit_cast is available])
  else
    AC_DEFINE(HAVE_STD_BIT_CAST, 0, [Define if std::bit_cast is available])
  fi
])

AX_CXX_STD_CXX_FLAG
AX_CXX17_LIBSTDCPPFS
AX_CXX20_BIT_CAST
