AC_ARG_WITH(drmingw,
  AS_HELP_STRING([--with-drmingw=path],[use link against Dr. MinGW's crash reporting library found in path]),
  [ drmingw_path="$with_drmingw" ],)

USE_DRMINGW=no

if ! test -z "$drmingw_path"; then
  AC_CACHE_CHECK([for Dr. MinGW], [ax_cv_drmingw_found], [
    save_CFLAGS="$CFLAGS"
    save_LIBS="$LIBS"
    CFLAGS="$CFLAGS -I$drmingw_path/include"
    LIBS="$LIBS -L$drmingw_path/lib -lexchndl"
    AC_LINK_IFELSE([AC_LANG_PROGRAM(
        [[#include "exchndl.h"]],
        [[ExcHndlInit();]])],
      [ax_cv_drmingw_found=yes],
      [ax_cv_drmingw_found=no])
    CFLAGS="$save_CFLAGS"
    LIBS="$save_LIBS"
  ])

  if test x"$ax_cv_drmingw_found" = xyes; then
    DRMINGW_PATH="$drmingw_path"
    USE_DRMINGW=yes
  fi
fi

AC_SUBST(DRMINGW_PATH)
AC_SUBST(USE_DRMINGW)
