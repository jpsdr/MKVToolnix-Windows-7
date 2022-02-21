dnl
dnl Check for Qt 5
dnl

qt_min_ver=5.9.0

check_qt5() {
  AC_ARG_WITH(qmake,
    AC_HELP_STRING([--with-qmake=prog],[use prog instead of looking for qmake for Qt 5]),
    [ QMAKE="$with_qmake" ],)

  if ! test -z "$QMAKE"; then
    AC_MSG_CHECKING(for qmake)
    AC_MSG_RESULT(using supplied $QMAKE)
  else
    AC_PATH_TOOL(QMAKE, qmake-qt5,, $PATH)
    if test -z "$QMAKE"; then
      AC_PATH_TOOL(QMAKE, qmake,, $PATH)
    fi
  fi

  if test x"$QMAKE" = x; then
    AC_MSG_CHECKING(for Qt 5)
    AC_MSG_RESULT(no: qmake not found)
    return
  fi

  qmake_properties="`mktemp`"

  "$QMAKE" -query > "$qmake_properties"

  qmake_ver="`$ac_cv_path_EGREP '^QT_VERSION:' "$qmake_properties" | sed 's/^QT_VERSION://'`"

  AC_MSG_CHECKING(for qmake's version)
  if test -z "qmake_ver"; then
    AC_MSG_RESULT(unknown; please contact the author)
    return
  elif ! check_version $qt_min_ver $qmake_ver; then
    AC_MSG_RESULT(too old: $qmake_ver, neet at least $qt_mIN-ver)
    return
  else
    AC_MSG_RESULT($qmake_ver)
  fi

  qt_bindir="`$ac_cv_path_EGREP '^QT_INSTALL_BINS:' "$qmake_properties" | sed 's/^QT_INSTALL_BINS://'`"
  qt_libexecdir="`$ac_cv_path_EGREP '^QT_INSTALL_LIBEXECS:' "$qmake_properties" | sed 's/^QT_INSTALL_LIBEXECS://'`"
  qt_searchpath="$qt_libexecdir:$qt_bindir:$PATH"

  rm -f "$qmake_properties"

  AC_PATH_PROG(LCONVERT, lconvert,, [$qt_searchpath])
  AC_PATH_PROG(MOC, moc,, [$qt_searchpath])
  AC_PATH_PROG(RCC, rcc,, [$qt_searchpath])
  AC_PATH_PROG(UIC, uic,, [$qt_searchpath])

  if test x"$MOC" = x; then
    AC_MSG_CHECKING(for Qt 5)
    AC_MSG_RESULT(no: could not find the moc executable)
    return

  elif test x"$RCC" = x; then
    AC_MSG_CHECKING(for Qt 5)
    AC_MSG_RESULT(no: could not find the rcc executable)
    return

  elif test x"$UIC" = x; then
    AC_MSG_CHECKING(for Qt 5)
    AC_MSG_RESULT(no: could not find the uic executable)
    return
  fi


  if test x"$enable_static_qt" = "xyes"; then
    QT_PKG_CONFIG_STATIC=--static
  else
    QT_PKG_CONFIG_STATIC=
  fi

  if test "x$with_qt_pkg_config" = xyes; then
    with_qt_pkg_config_modules="`echo "$with_qt_pkg_config_modules" | sed -e 's/ /,/g'`"
    if test x"$with_qt_pkg_config_modules" != x ; then
      with_qt_pkg_config_modules="$with_qt_pkg_config_modules,"
    fi

    orig_with_qt_pkg_config_modules="$with_qt_pkg_config_modules,"

    with_qt_pkg_config_modules="$with_qt_pkg_config_modules,Qt5Core"

    if test x"$enable_gui" = xyes; then
        with_qt_pkg_config_modules="$with_qt_pkg_config_modules,Qt5Gui,Qt5Widgets,Qt5Multimedia,Qt5Network,Qt5Concurrent"
    fi

    if test x"$MINGW" = x1; then
      with_qt_pkg_config_modules="$with_qt_pkg_config_modules"
    fi

    PKG_CHECK_EXISTS([$with_qt_pkg_config_modules],[ok=1],[ok=0])
    PKG_CHECK_EXISTS([Qt5PlatformSupport],[with_qt_pkg_config_modules="$with_qt_pkg_config_modules,Qt5PlatformSupport"])

    if test $ok = 0; then
      AC_MSG_CHECKING(for Qt 5)
      AC_MSG_RESULT(no: not found by pkg-config)
      return
    fi

    if test x"$MINGW" != x1 && ! echo "$host" | grep -q -i apple ; then
      PKG_CHECK_EXISTS([Qt5DBus],[dbus_found=yes],[dbus_found=no])
      if test x"$dbus_found" = xyes; then
        with_qt_pkg_config_modules="$with_qt_pkg_config_modules,Qt5DBus"
        AC_DEFINE(HAVE_QTDBUS, 1, [Define if QtDBus is present])
      fi
    fi

    with_qt_pkg_config_modules="`echo "$with_qt_pkg_config_modules" | sed -e 's/,/ /g'`"
    QT_CFLAGS="`$PKG_CONFIG --cflags $with_qt_pkg_config_modules $QT_PKG_CONFIG_STATIC`"
    QT_LIBS="`$PKG_CONFIG --libs $with_qt_pkg_config_modules $QT_PKG_CONFIG_STATIC`"
    QT_LIBS_NON_GUI="`$PKG_CONFIG --libs $orig_with_qt_pkg_config_modules,Qt5Core $QT_PKG_CONFIG_STATIC`"
  fi

  dnl compile test program
  AC_LANG_PUSH(C++)
  AC_CACHE_VAL(am_cv_qt5_compilation, [
    run_qt_test=1
    while true; do
      ac_save_CXXFLAGS="$CXXFLAGS"
      ac_save_LIBS="$LIBS"
      CXXFLAGS="$STD_CXX $CXXFLAGS $QT_CFLAGS -fPIC"
      LIBS="$LDFLAGS $QT_LIBS"
      unset ac_cv_qt_compilation

      AC_TRY_LINK([
#include <QtCore>
#include <QCoreApplication>
class Config : public QCoreApplication {
public:
Config(int &argc, char **argv);
};
Config::Config(int &argc, char **argv)
: QCoreApplication(argc,argv) {setApplicationName("config");}
        ], [
int ai = 0;
char **ac = 0;
Config app(ai,ac);
return 0;
        ], [ am_cv_qt5_compilation=1 ], [ am_cv_qt5_compilation=0 ])

      CXXFLAGS="$ac_save_CXXFLAGS"
      LIBS="$ac_save_LIBS"

      if test x"$am_cv_qt5_compilation" = x1; then
        break

      elif test x"$run_qt_test" = "x1"; then
        QT_CFLAGS="$QT_CFLAGS -I/usr/include/QtCore -I/usr/include/QtGui -I/usr/include/QtWidgets -I/usr/local/include/QtCore -I/usr/local/include/QtGui -I/usr/local/include/QtWidgets -I/usr/local/include/QtNetwork -I/usr/local/include/QtPlatformSupport"
        run_qt_test=3

      else
        break

      fi
    done
    ])
  AC_LANG_POP()

  rm -f src/mkvtoolnix-gui/static_plugins.cpp

  if ! test x"$am_cv_qt5_compilation" = x1; then
    AC_MSG_CHECKING(for Qt 5)
    AC_MSG_RESULT(no: test program could not be compiled)
    return
  fi

  if test x"$QT_PKG_CONFIG_STATIC" != x; then
    qmake_dir="`mktemp -d`"

    if test x"$MINGW" = x1 && check_version 5.10.0 $moc_ver; then
      QTPLUGIN="qwindowsvistastyle"
    else
      QTPLUGIN=""
    fi

    touch "$qmake_dir/empty.cpp"
    cat > "$qmake_dir/dummy.pro" <<EOF
QT += core multimedia
QTPLUGIN += $QTPLUGIN
CONFIG += release static
TARGET = console
TEMPLATE = app
SOURCES += empty.cpp
EOF

    old_wd="$PWD"
    cd "$qmake_dir"

    "$QMAKE" -makefile -nocache dummy.pro > /dev/null 2>&1
    result=$?

    cd "$old_wd"

    makefile=""
    if test x$result != x0; then
      problem="qmake failed to create Makefile"

    elif ! test -f "$qmake_dir/console_plugin_import.cpp"; then
      problem="static plugin list could not be generated via $QMAKE"

    elif test -f "$qmake_dir/Makefile.Release"; then
      makefile="$qmake_dir/Makefile.Release"

    elif test -f "$qmake_dir/Makefile"; then
      makefile="$qmake_dir/Makefile"

    else
      problem="the Makefile created by $QMAKE could not be found"
    fi

    if test x"$problem" = x; then
      qmake_libs="`grep '^LIBS' "$makefile" | sed -Ee 's/^LIBS[[ \t]]*=[[ \t]]*//'`"
      QT_LIBS="$qmake_libs $QT_LIBS"

      cp "$qmake_dir/console_plugin_import.cpp" src/mkvtoolnix-gui/static_plugins.cpp
    fi

    rm -rf "$qmake_dir"

    unset makefile qmake_libs qmake_dir
  fi

  AC_MSG_CHECKING(for Qt 5)

  if test x"$problem" = x; then
    AC_DEFINE(HAVE_QT, 1, [Define if Qt is present])
    AC_MSG_RESULT(yes)
    have_qt5=yes
  else
    AC_MSG_RESULT(no: $problem)
  fi

  unset problem
}

AC_ARG_ENABLE([qt5],
  AC_HELP_STRING([--enable-qt5],[compile with Qt 5 (yes if Qt 6 is not found)]),
  [],[enable_qt5=yes])
AC_ARG_ENABLE([static_qt],
  AC_HELP_STRING([--enable-static-qt],[link to static versions of the Qt library (no)]))
AC_ARG_WITH([qt_pkg_config_modules],
  AC_HELP_STRING([--with-qt-pkg-config-modules=modules],[gather include/link flags for additional Qt 5 modules from pkg-config]))
AC_ARG_WITH([qt_pkg_config],
  AC_HELP_STRING([--without-qt-pkg-config], [do not use pkg-config for detecting Qt 5; instead rely on QT_CFLAGS/QT_LIBS being set correctly already]),
  [ with_qt_pkg_config=${withval} ], [ with_qt_pkg_config=yes ])

have_qt5=no

if test x"$have_qt6" = "xyes"; then
  AC_MSG_CHECKING(for Qt 5)
  AC_MSG_RESULT(no: already using Qt 6)

elif test x"$enable_qt5" != xyes; then
  AC_MSG_CHECKING(for Qt 5)
  AC_MSG_RESULT(no: disabled by user request)

else
  check_qt5
fi
