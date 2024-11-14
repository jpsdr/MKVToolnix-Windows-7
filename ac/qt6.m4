dnl
dnl Check for Qt 6
dnl

qt_min_ver=6.2.0

check_qt6() {
  AC_ARG_WITH(qmake6,
    AS_HELP_STRING([--with-qmake6=prog],[use prog instead of looking for qmake6]),
    [ QMAKE6="$with_qmake6" ],)

  if ! test -z "$QMAKE6"; then
    AC_MSG_CHECKING(for qmake6)
    AC_MSG_RESULT(using supplied $QMAKE6)
  else
    AC_CHECK_TOOLS(QMAKE6, [ qmake6 qmake-qt6 qt6-qmake ])
  fi

  if test x"$QMAKE6" = x; then
    AC_MSG_CHECKING(for Qt 6)
    AC_MSG_RESULT(no: qmake6 not found)
    return
  fi

  QMAKE_SPEC=""
  if "$QMAKE6" -query 2>&5 | grep -F -q "QMAKE_XSPEC:linux-g++"; then
    if test x"$COMPILER_TYPE" = xclang; then
      QMAKE_SPEC="-spec linux-clang"
    else
      QMAKE_SPEC="-spec linux-g++"
    fi
  fi

  rm -f src/mkvtoolnix-gui/static_plugins.cpp
  qmake_dir="`mktemp -d "${TMPDIR:-/tmp}/tmp.mkvtoolnix.XXXXXX"`"

  touch "$qmake_dir/configure.cpp" "$qmake_dir/configure.h"

  cat > "$qmake_dir/configure.ui" <<EOT
<?xml version="1.0" encoding="UTF-8"?>
<ui version="4.0">
</ui>
EOT

  cat > "$qmake_dir/configure.qrc" <<EOT
<?xml version='1.0' encoding='UTF-8'?>
<!DOCTYPE RCC>
<RCC version='1.0'>
 <qresource>
  <file>configure.pro</file>
 </qresource>
</RCC>
EOT

  qmake_qtplugin_ui=""
  qmake_qt_ui=""

  cat > "$qmake_dir/configure_non_gui.pro" <<EOT
QT = core
TARGET = console

HEADERS = configure.h
SOURCES = configure.cpp
EOT

  old_wd="$PWD"
  cd "$qmake_dir"

  "$QMAKE6" -makefile -nocache $QMAKE_SPEC configure_non_gui.pro 2>&5 > /dev/null
  result=$?

  if test $result = 0; then
    if test -f Makefile.Release; then
      mv Makefile.Release Makefile.non_gui
    else
      mv Makefile Makefile.non_gui
    fi
  fi

  have_qtdbus=no
  modules_to_test=dbus

  if test x"$enable_gui" = xno; then
    modules_to_test=
  fi

  for qt_module in $modules_to_test; do
    rm -f Makefile Makefile.Release

    cat > "$qmake_dir/configure.pro" <<EOT
QT = core $qt_module gui widgets network concurrent svg
QTPLUGIN += $qmake_qtplugin_ui

FORMS = configure.ui
RESOURCES = configure.qrc
HEADERS = configure.h
SOURCES = configure.cpp
EOT

    "$QMAKE6" -makefile -nocache $QMAKE_SPEC configure.pro 2>&5 > /dev/null
    result2=$?

    if test $result2 != 0; then
      continue
    elif test $qt_module = dbus; then
      qmake_qt_ui="$qmake_qt_ui dbus"
      have_qtdbus=yes
    fi
  done

  rm -f Makefile Makefile.Release

  modules_to_test="gui widgets network concurrent svg multimedia"
  if test x"$enable_gui" = xno; then
    modules_to_test=
  fi

  cat > "$qmake_dir/configure.pro" <<EOT
QT = core $qmake_qt_ui $modules_to_test
QTPLUGIN += $qmake_qtplugin_ui

FORMS = configure.ui
RESOURCES = configure.qrc
HEADERS = configure.h
SOURCES = configure.cpp
EOT

  "$QMAKE6" -makefile -nocache $QMAKE_SPEC configure.pro 2>&5 > /dev/null
  result2=$?

  if test $result2 != 0; then
    cd "$old_wd"
    AC_MSG_RESULT(no: not all of the required Qt6 modules were found (needed: core $modules_to_test))
    return
  fi

  if test -f Makefile.Release; then
    mv Makefile.Release Makefile
  fi
  if test -f configure_plugin_import.cpp; then
    sed -i -e 's/Q_IMPORT_PLUGIN[(]QWindowsVistaStylePlugin[)]//' configure_plugin_import.cpp
    cp configure_plugin_import.cpp "$old_wd/src/mkvtoolnix-gui/static_plugins.cpp"
  fi

  "$QMAKE6" -query $QMAKE_SPEC > "$qmake_dir/configure.properties" 2>&5
  result3=$?

  cd "$old_wd"

  if test $result != 0 -o $result2 != 0 -o $result3 != 0; then
    AC_MSG_CHECKING(for Qt 6)
    AC_MSG_RESULT(no: qmake6 couldn't be run for a dummy project)

    rm -rf "$qmake_dir"

    return
  fi

  dnl Check its version.
  qmake6_ver="`$ac_cv_path_EGREP '^QT_VERSION:' "$qmake_dir/configure.properties" | sed 's/^QT_VERSION://'`"

  AC_MSG_CHECKING(for qmake6's version)
  if test -z "qmake6_ver"; then
    AC_MSG_RESULT(unknown; please contact the author)
    return
  elif ! check_version $qt_min_ver $qmake6_ver; then
    AC_MSG_RESULT(too old: $qmake6_ver, need at least $qt_min_ver)
    return
  else
    AC_MSG_RESULT($qmake6_ver)
  fi

  qt_bindir="`$ac_cv_path_EGREP '^QT_HOST_BINS:' "$qmake_dir/configure.properties" | sed 's/^QT_HOST_BINS://'`"
  qt_libexecdir="`$ac_cv_path_EGREP '^QT_HOST_LIBEXECS:' "$qmake_dir/configure.properties" | sed 's/^QT_HOST_LIBEXECS://'`"

  # If under MinGW/MSYS2, convert these two paths to Unix style
  if ! test -z "${MINGW_PREFIX}"; then
    qt_bindir="`cygpath -u ${qt_bindir}`"
    qt_libexecdir="`cygpath -u ${qt_libexecdir}`"
  fi

  qt_searchpath="$qt_libexecdir:$qt_bindir:$PATH"

  QT_CFLAGS="`$ac_cv_path_EGREP '^DEFINES *=' "$qmake_dir/Makefile" | sed 's/^DEFINES *= *//'`"
  QT_CFLAGS="$QT_CFLAGS `$ac_cv_path_EGREP '^CXXFLAGS *=' "$qmake_dir/Makefile" | sed -e 's/^CXXFLAGS *= *//' -e 's/-pipe//g' -e 's/-O.//g' -e 's/ -f[[a-z]][[^ ]]*//g' -e 's/ -W[[^ ]]*//g' -e 's/-std=[[^ ]]*//g' -e 's/\$([[^)]]*)//g'`"
  QT_INCFLAGS="`$ac_cv_path_EGREP '^INCPATH *=' "$qmake_dir/Makefile" | sed -e 's/^INCPATH *= *//'`"

  # If under MinGW/MSYS2, fix relative include paths
  if ! test -z "${MINGW_PREFIX}"; then
    QT_INCFLAGS="`echo $QT_INCFLAGS | sed -e "s:../..${MINGW_PREFIX}:${MINGW_PREFIX}:g"`"
  fi

  QT_INCFLAGS="`echo $QT_INCFLAGS | sed -e 's:-I[[^/]][[^ ]]*::g'`"
  QT_CFLAGS="$QT_CFLAGS $QT_INCFLAGS"
  QT_CFLAGS="`echo $QT_CFLAGS | sed -e 's/\$(EXPORT_ARCH_ARGS)//'`"
  QT_LIBS="`$ac_cv_path_EGREP '^LFLAGS *=' "$qmake_dir/Makefile" | sed -e 's/^LFLAGS *= *//' -e 's/-Wl,-O[[^ ]]*//g' -e 's/ -f[[a-z]][[^ ]]*//g'`"
  QT_LIBS="$QT_LIBS `$ac_cv_path_EGREP '^LIBS *=' "$qmake_dir/Makefile" | sed -e 's/^LIBS *= *//' -e 's/\$([[^)]]*)//g' -e 's:-L[[^/]][[^ ]]*::g'`"
  QT_LIBS="`echo $QT_LIBS | sed -e 's/\$(EXPORT_ARCH_ARGS)//'`"
  QT_LIBS_NON_GUI="`$ac_cv_path_EGREP '^LFLAGS *=' "$qmake_dir/Makefile.non_gui" | sed -e 's/^LFLAGS *= *//' -e 's/-Wl,-O[[^ ]]*//g' -e 's/ -f[[a-z]][[^ ]]*//g'`"
  QT_LIBS_NON_GUI="$QT_LIBS_NON_GUI `$ac_cv_path_EGREP '^LIBS *=' "$qmake_dir/Makefile.non_gui" | sed -e 's/^LIBS *= *//' -e 's/\$([[^)]]*)//g' -e 's:-L[[^/]][[^ ]]*::g'`"
  QT_LIBS_NON_GUI="`echo $QT_LIBS_NON_GUI | sed -e 's/\$(EXPORT_ARCH_ARGS)//' -e 's/-Wl,-subsystem,windows *//g'`"

  rm -rf "$qmake_dir"

  if test x"$QT_CFLAGS" = x -o x"$QT_LIBS" = x -o x"$QT_LIBS_NON_GUI" = x; then
    AC_MSG_CHECKING(for Qt 6)
    AC_MSG_RESULT(no: could not extract one or more compiler flags from Makefile generated by qmake6)
    return
  fi

  AC_PATH_PROG(LCONVERT, lconvert,, [$qt_searchpath])
  AC_PATH_PROG(MOC, moc,, [$qt_searchpath])
  AC_PATH_PROG(RCC, rcc,, [$qt_searchpath])
  AC_PATH_PROG(UIC, uic,, [$qt_searchpath])

  if test x"$MOC" = x; then
    AC_MSG_CHECKING(for Qt 6)
    AC_MSG_RESULT(no: could not find the moc executable)
    return

  elif test x"$RCC" = x; then
    AC_MSG_CHECKING(for Qt 6)
    AC_MSG_RESULT(no: could not find the rcc executable)
    return

  elif test x"$UIC" = x; then
    AC_MSG_CHECKING(for Qt 6)
    AC_MSG_RESULT(no: could not find the uic executable)
    return
  fi

  dnl compile test program
  AC_LANG_PUSH(C++)
  AC_CACHE_VAL(am_cv_qt6_compilation, [
    ac_save_CXXFLAGS="$CXXFLAGS"
    ac_save_LIBS="$LIBS"
    CXXFLAGS="$STD_CXX $CXXFLAGS $QT_CFLAGS -fPIC"
    LIBS="$LDFLAGS $QT_LIBS"
    unset ac_cv_qt_compilation

    AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <QtCore>
#include <QCoreApplication>
class Config : public QCoreApplication {
public:
Config(int &argc, char **argv);
};
Config::Config(int &argc, char **argv)
: QCoreApplication(argc,argv) {setApplicationName("config");}
      ]], [[
int ai = 0;
char **ac = 0;
Config app(ai,ac);
return 0;
      ]])],[ am_cv_qt6_compilation=1 ],[ am_cv_qt6_compilation=0 ])

    CXXFLAGS="$ac_save_CXXFLAGS"
    LIBS="$ac_save_LIBS"
  ])
  AC_LANG_POP()

  if test x"$am_cv_qt6_compilation" != x1; then
    AC_MSG_CHECKING(for Qt 6)
    AC_MSG_RESULT(no: could not compile a test program)
    return
  fi

  AC_DEFINE(HAVE_QT, 1, [Define if Qt is present])
  AC_MSG_CHECKING(for Qt 6)
  AC_MSG_RESULT(yes)
  have_qt6=yes
}

AC_ARG_ENABLE([gui],
  AS_HELP_STRING([--enable-gui],[compile the Qt-based GUI (yes)]),
  [],[enable_gui=yes])

AC_ARG_ENABLE([dbus],
  AS_HELP_STRING([--enable-dbus],[compile DBus support (auto)]),
  [],[enable_dbus=auto])

have_qt6=no

check_qt6

unset qmake_dir qt_bindir qt_libdir qt_searchpath

if test $have_qt6 != yes; then
  AC_MSG_ERROR([The Qt library version >= $qt_min_ver is required for building MKVToolNix.])
fi

if test x"$enable_gui" = xyes; then
  BUILD_GUI=yes
  opt_features_yes="$opt_features_yes\n   * MKVToolNix GUI"

else
  BUILD_GUI=no
  opt_features_no="$opt_features_no\n   * MKVToolNix GUI"
fi

if test x"$enable_dbus" = xno; then
  opt_features_no="$opt_features_no\n   * DBus support"
elif test "x$have_qtdbus" = xyes; then
  # have_dbus == yes && enable_dbus != no
  opt_features_yes="$opt_features_yes\n   * DBus support"
  AC_DEFINE(HAVE_QTDBUS, 1, [Define if QtDBus is present])
elif test x"$enable_dbus" = xyes; then
  # have_dbus == no && enable_dbus == yes
  AC_MSG_ERROR([QtDBus not found.])
else
  # have_dbus == no && enable_dbus == auto
  opt_features_no="$opt_features_no\n   * DBus support"
fi


AC_SUBST(QT_CFLAGS)
AC_SUBST(QT_LIBS)
AC_SUBST(QT_LIBS_NON_GUI)
AC_SUBST(BUILD_GUI)
