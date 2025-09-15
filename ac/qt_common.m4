if test x"$have_qt6" != xyes -a x"$have_qt5" != xyes; then
  AC_MSG_ERROR([The Qt library is required for building MKVToolNix.])
fi

if test x"$enable_gui" = xyes; then
  BUILD_GUI=yes

  if test x"$have_qt6" = "xyes" ; then
    opt_features_yes="$opt_features_yes\n   * MKVToolNix GUI (with Qt 6)"
  else
    opt_features_yes="$opt_features_yes\n   * MKVToolNix GUI (with Qt 5)"
  fi

else
  BUILD_GUI=no

  opt_features_no="$opt_features_no\n   * MKVToolNix GUI"
fi

AC_SUBST(QT_CFLAGS)
AC_SUBST(QT_LIBS)
AC_SUBST(QT_LIBS_NON_GUI)
AC_SUBST(BUILD_GUI)
