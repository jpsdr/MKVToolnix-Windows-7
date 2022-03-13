dnl
dnl Online check for updates
dnl

AC_ARG_ENABLE(
  [update-check],
  AC_HELP_STRING([--enable-update-check],[If enabled, the GUI will check online for available updates daily by downloading a small XML file from the MKVToolNix home page (default: yes)]),
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
dnl mkvtoolnix wrapper binary
dnl

AC_ARG_ENABLE(
  [mkvtoolnix],
  AC_HELP_STRING([--enable-mkvtoolnix],[If enabled, the 'mkvtoolnix' wrapper binary will be built (default: no)]),
  [BUILD_MKVTOOLNIX=$enable_mkvtoolnix],
  [BUILD_MKVTOOLNIX=no]
)

if test x"$BUILD_MKVTOOLNIX" = xyes ; then
  opt_features_yes="$opt_features_yes\n   * 'mkvtoolnix' wrapper binary"
else
  opt_features_no="$opt_features_no\n   * 'mkvtoolnix' wrapper binary"
fi

AC_SUBST(BUILD_MKVTOOLNIX)
