dnl
dnl Test for libEBML and libMatroska, and define MATROSKA_CFLAGS and MATROSKA_LIBS
dnl

EBML_MATROSKA_INTERNAL=no
PKG_CHECK_MODULES([EBML],[libebml >= 1.3.5],[],[EBML_MATROSKA_INTERNAL=yes])
PKG_CHECK_MODULES([MATROSKA],[libmatroska >= 1.4.8],[],[EBML_MATROSKA_INTERNAL=yes])

if test x"$EBML_MATROSKA_INTERNAL" = xyes; then
  if ! test -f lib/libebml/ebml/EbmlTypes.h ; then
    echo '*** The internal version of the libEBML library is supposed to be used,'
    echo '*** but it was not found in "lib/libebml. If this is a clone from the'
    echo '*** git repository then submodules have to be initialized with the'
    echo '*** following two commands:'
    echo '***'
    echo '*** git submodule init'
    echo '*** git submodule update'
    exit 1
  fi

  if ! test -f lib/libmatroska/matroska/KaxVersion.h ; then
    echo '*** The internal version of the libMatroska library is supposed to be used,'
    echo '*** but it was not found in "lib/libmatroska". If this is a clone from the'
    echo '*** git repository then submodules have to be initialized with the'
    echo '*** following two commands:'
    echo '***'
    echo '*** git submodule init'
    echo '*** git submodule update'
    exit 1
  fi
fi

AC_DEFUN([AX_EBML_FINDNEXTCHILD],[
  AC_CACHE_CHECK([if libEBML contains FindNextChild], [ax_cv_ebml_findnextchild],[
    CXXFLAGS_SAVED=$CXXFLAGS

    if test x"$EBML_MATROSKA_INTERNAL" = xyes; then
      CXXFLAGS="-Ilib/libebml -Ilib/libmatroska"
    else
      CXXFLAGS="$EBML_CFLAGS $MATROSKA_CFLAGS"
    fi
    export CXXFLAGS

    AC_LANG_PUSH(C++)
    AC_TRY_COMPILE(
      [
#include <matroska/KaxInfo.h>
#include <matroska/KaxSegment.h>

using namespace libebml;
using namespace libmatroska;
      ],
      [
  EbmlMaster *m = new KaxSegment;
  EbmlElement *e = new KaxTitle;
  FindNextChild(*m, *e);
  ],
      [ax_cv_ebml_findnextchild="yes"],
      [ax_cv_ebml_findnextchild="no"])
    AC_LANG_POP

    CXXFLAGS="$CXXFLAGS_SAVED"
  ])

  if test x"$ax_cv_ebml_findnextchild" = xyes; then
    AC_DEFINE(HAVE_EBML_FINDNEXTCHILD, 1, [Define if libEBML contains FindNextChild])
  fi
])

AX_EBML_FINDNEXTCHILD

AC_SUBST(EBML_MATROSKA_INTERNAL)
AC_SUBST(EBML_CFLAGS)
AC_SUBST(EBML_LIBS)
AC_SUBST(MATROSKA_CFLAGS)
AC_SUBST(MATROSKA_LIBS)
