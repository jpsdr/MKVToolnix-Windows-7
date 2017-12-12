dnl
dnl Online check for updates
dnl

AC_ARG_ENABLE(
  [update-check],
  AC_HELP_STRING([--enable-update-check],[If enabled, the GUI will check online for available updates daily by downloading a small XML file from the MKVToolNix home page (yes)]),
  [],
  [enable_update_check=yes]
)

if test x"$enable_update_check" = xyes ; then
  AC_DEFINE([HAVE_UPDATE_CHECK],[1],[Defined if the online update check is enabled])
  opt_features_yes="$opt_features_yes\n   * online update check in the GUI"
else
  opt_features_no="$opt_features_no\n   * online update check in the GUI"
fi


dnl
dnl AppImage build
dnl

AC_ARG_ENABLE(
  [appimage],
  AC_HELP_STRING([--enable-appimage],[Enable certain relocation settings useful for building AppImages or generic portable builds (no)]),
  [enable_appimage=yes],
  [enable_appimage=no]
)

if test x"$enable_appimage" = xyes ; then
  opt_features_yes="$opt_features_yes\n   * AppImage compatibility"
  APPIMAGE_BUILD="yes"
else
  opt_features_no="$opt_features_no\n   * AppImage compatibility"
fi

AC_SUBST(APPIMAGE_BUILD)

