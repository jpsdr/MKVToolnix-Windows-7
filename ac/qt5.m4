dnl
dnl Check for Qt 5
dnl

qt_min_ver=5.3.0

AC_ARG_ENABLE([qt],
  AC_HELP_STRING([--enable-qt],[compile the Qt version of the GUIs (yes)]),
  [],[enable_qt=yes])
AC_ARG_ENABLE([static_qt],
  AC_HELP_STRING([--enable-static-qt],[link to static versions of the Qt library (no)]))
AC_ARG_WITH([qt_pkg_config_modules],
  AC_HELP_STRING([--with-qt-pkg-config-modules=modules],[gather include/link flags for additional Qt modules from pkg-config]))
AC_ARG_WITH([qt_pkg_config],
  AC_HELP_STRING([--without-qt-pkg-config], [do not use pkg-config for detecting Qt; instead rely on QT_CFLAGS/QT_LIBS being set correctly already]),
  [ with_qt_pkg_config=${withval} ], [ with_qt_pkg_config=yes ])

if test x"$enable_qt" = "xyes" -a \
  '(' x"$enable_gui" = x"yes" -o x"$enable_gui" = "x" ')'; then
  if test x"$enable_static_qt" = "xyes"; then
    AC_DEFINE(HAVE_STATIC_QT,,[define if building against a static Qt library])
    QT_PKG_CONFIG_STATIC=--static
  else
    QT_PKG_CONFIG_STATIC=
  fi

  dnl Find moc.
  AC_ARG_WITH(moc,
    AC_HELP_STRING([--with-moc=prog],[use prog instead of looking for moc]),
    [ MOC="$with_moc" ],)
  if ! test -z "$MOC"; then
    AC_MSG_CHECKING(for moc)
    AC_MSG_RESULT(using supplied $MOC)
  else
    AC_PATH_PROG(MOC, moc-qt5,, $PATH)
    if test -z "$MOC"; then
      AC_PATH_PROG(MOC, moc,, $PATH)
    fi
  fi

  if test -n "$MOC" -a -x "$MOC"; then
    dnl Check its version.
    AC_MSG_CHECKING(for the Qt version $MOC uses)
    moc_ver=`"$MOC" -v 2>&1 | sed -e 's:.*Qt ::' -e 's:.* ::' -e 's:[[^0-9\.]]::g'`
    if test -z "moc_ver"; then
      AC_MSG_RESULT(unknown; please contact the author)
    elif ! check_version $qt_min_ver $moc_ver; then
      AC_MSG_RESULT(too old: $moc_ver)
    else
      AC_MSG_RESULT($moc_ver)
      moc_found=1
    fi
  fi

  AC_ARG_WITH(uic,
    AC_HELP_STRING([--with-uic=prog],[use prog instead of looking for uic]),
    [ UIC="$with_uic" ],)

  if ! test -z "$UIC"; then
    AC_MSG_CHECKING(for uic)
    AC_MSG_RESULT(using supplied $UIC)
  else
    AC_PATH_PROG(UIC, uic-qt5,, $PATH)
    if test -z "$UIC"; then
      AC_PATH_PROG(UIC, uic,, $PATH)
    fi
  fi

  if test -n "$UIC" -a -x "$UIC"; then
    dnl Check its version.
    AC_MSG_CHECKING(for the Qt version $UIC uses)
    uic_ver=`"$UIC" -v 2>&1 | sed -e 's:.*Qt ::' -e 's:.* ::' -e 's:[[^0-9\.]]::g'`
    if test -z "uic_ver"; then
      AC_MSG_RESULT(unknown; please contact the author)
    elif ! check_version $qt_min_ver $uic_ver; then
      AC_MSG_RESULT(too old: $uic_ver)
    else
      AC_MSG_RESULT($uic_ver)
      uic_found=1
    fi
  fi

  AC_ARG_WITH(rcc,
    AC_HELP_STRING([--with-rcc=prog],[use prog instead of looking for rcc]),
    [ RCC="$with_rcc" ],)

  if ! test -z "$RCC"; then
    AC_MSG_CHECKING(for rcc)
    AC_MSG_RESULT(using supplied $RCC)
  else
    AC_PATH_PROG(RCC, rcc-qt5,, $PATH)
    if test -z "$RCC"; then
      AC_PATH_PROG(RCC, rcc,, $PATH)
    fi
  fi

  if test -n "$RCC" -a -x "$RCC"; then
    dnl Check its version.
    AC_MSG_CHECKING(for the Qt version $RCC uses)
    rcc_ver=`"$RCC" -v 2>&1 | sed -e 's:.*Qt ::' -e 's:.* ::' -e 's:[[^0-9\.]]::g'`
    if test -z "rcc_ver"; then
      AC_MSG_RESULT(unknown; please contact the author)
    elif ! check_version $qt_min_ver $rcc_ver; then
      AC_MSG_RESULT(too old: $rcc_ver)
    else
      AC_MSG_RESULT($rcc_ver)
      rcc_found=1
    fi
  fi

  AC_ARG_WITH(qmake,
    AC_HELP_STRING([--with-qmake=prog],[use prog instead of looking for qmake]),
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

  if test -n "$QMAKE" -a -x "$QMAKE"; then
    dnl Check its version.
    AC_MSG_CHECKING(for the Qt version $QMAKE uses)
    qmake_ver=`LC_ALL=C "$QMAKE" -v 2>&1 | grep 'Using Qt' | sed -e 's:.*version ::' -e 's: .*::'`
    if test -z "qmake_ver"; then
      AC_MSG_RESULT(unknown; please contact the author)
    elif ! check_version $qt_min_ver $qmake_ver; then
      AC_MSG_RESULT(too old: $qmake_ver)
    else
      AC_MSG_RESULT($qmake_ver)
      qmake_found=1
    fi
  fi

  ok=0
  AC_MSG_CHECKING(for Qt $qt_min_ver or newer)
  if test x"$moc_found" != "x1"; then
    AC_MSG_RESULT(no: moc not found or too old)
  elif test x"$uic_found" != "x1"; then
    AC_MSG_RESULT(no: uic not found or too old)
  elif test x"$rcc_found" != "x1"; then
    AC_MSG_RESULT(no: rcc not found or too old)
  elif test x"$qmake_found" != "x1"; then
    AC_MSG_RESULT(no: qmake not found or too old)
  else
    ok=1
  fi

  if test $ok = 1 -a "x$with_qt_pkg_config" = xyes; then
    with_qt_pkg_config_modules="`echo "$with_qt_pkg_config_modules" | sed -e 's/ /,/g'`"
    if test x"$with_qt_pkg_config_modules" != x ; then
      with_qt_pkg_config_modules="$with_qt_pkg_config_modules,"
    fi

    with_qt_pkg_config_modules="$with_qt_pkg_config_modules,Qt5Core,Qt5Gui,Qt5Widgets,Qt5Multimedia,Qt5Network,Qt5Concurrent"

    if test x"$MINGW" = x1; then
      with_qt_pkg_config_modules="$with_qt_pkg_config_modules,Qt5WinExtras"
    fi

    PKG_CHECK_EXISTS([$with_qt_pkg_config_modules],,[ok=0])
    PKG_CHECK_EXISTS([Qt5PlatformSupport],[with_qt_pkg_config_modules="$with_qt_pkg_config_modules,Qt5PlatformSupport"])

    if test $ok = 0; then
      AC_MSG_RESULT(no: not found by pkg-config)
    fi

    with_qt_pkg_config_modules="`echo "$with_qt_pkg_config_modules" | sed -e 's/,/ /g'`"
    QT_CFLAGS="`$PKG_CONFIG --cflags $with_qt_pkg_config_modules $QT_PKG_CONFIG_STATIC`"
    QT_LIBS="`$PKG_CONFIG --libs $with_qt_pkg_config_modules $QT_PKG_CONFIG_STATIC`"
  fi

  if test $ok = 1; then
    dnl Try compiling and linking an application.

    AC_LANG_PUSH(C++)
    AC_CACHE_VAL(am_cv_qt_compilation, [
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
          ], [ am_cv_qt_compilation=1 ], [ am_cv_qt_compilation=0 ])

        CXXFLAGS="$ac_save_CXXFLAGS"
        LIBS="$ac_save_LIBS"

        if test x"$am_cv_qt_compilation" = x1; then
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

    problem=""
    if ! test x"$am_cv_qt_compilation" = x1; then
      problem="test program could not be compiled"
    fi

    rm -f src/info/static_plugins.cpp src/mkvtoolnix-gui/static_plugins.cpp

    if test x"$problem" = x && test x"$QT_PKG_CONFIG_STATIC" != x; then
      qmake_dir="`mktemp -d`"

      touch "$qmake_dir/empty.cpp"
      cat > "$qmake_dir/dummy.pro" <<EOF
QT += core multimedia
CONFIG += release static
TARGET = console
TEMPLATE = app
SOURCES += empty.cpp
EOF

      old_wd="$PWD"
      cd "$qmake_dir"

      "$QMAKE" -makefile -nocache dummy.pro > /dev/null

      cd "$old_wd"

      makefile=""
      if ! test -f "$qmake_dir/console_plugin_import.cpp"; then
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

        cp "$qmake_dir/console_plugin_import.cpp" src/info/static_plugins.cpp
        cp "$qmake_dir/console_plugin_import.cpp" src/mkvtoolnix-gui/static_plugins.cpp
      fi

      rm -rf "$qmake_dir"

      unset makefile qmake_libs qmake_dir
    fi

    if test x"$problem" = x; then
     AC_DEFINE(HAVE_QT, 1, [Define if Qt is present])
     have_qt=yes
     USE_QT=yes
     opt_features_yes="$opt_features_yes\n   * GUIs"
     AC_MSG_RESULT(yes)
    else
      AC_MSG_RESULT(no: $problem)
    fi

    unset problem
  fi

  AC_PATH_PROG(LCONVERT, lconvert)

else
  echo '*** Not checking for Qt: disabled by user request'
fi

if test x"$have_qt" != "xyes" ; then
  opt_features_no="$opt_features_no\n   * GUIs"
  QT_CFLAGS=
  QT_LIBS=
  MOC=
  UIC=
fi

AC_SUBST(MOC)
AC_SUBST(UIC)
AC_SUBST(QT_CFLAGS)
AC_SUBST(QT_LIBS)
AC_SUBST(USE_QT)
