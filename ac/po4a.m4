TRANSLATE_PERCENT=0
PO4A_FLAGS="-M utf-8 -k $TRANSLATE_PERCENT"
PO4A_TRANSLATE_FLAGS="$PO4A_FLAGS -f docbook"

AC_ARG_WITH(po4a,
  AC_HELP_STRING([--with-po4a=prog],[use prog instead of looking for po4a]),
  [ PO4A="$with_po4a" ],)

if ! test -z "$PO4A"; then
  AC_MSG_CHECKING(for po4a)
  AC_MSG_RESULT(using supplied $PO4A)
else
  AC_PATH_PROG(PO4A, po4a)
fi

AC_ARG_WITH(po4a_translate,
  AC_HELP_STRING([--with-po4a-translate=prog],[use prog instead of looking for po4a-translate]),
  [ PO4A-TRANSLATE="$with_po4a_translate" ],)

if ! test -z "$PO4A_TRANSLATE"; then
  AC_MSG_CHECKING(for po4a-translate)
  AC_MSG_RESULT(using supplied $PO4A_TRANSLATE)
else
  AC_PATH_PROG(PO4A_TRANSLATE, po4a-translate)
fi

if test "$PO4A" != "" -a "$PO4A_TRANSLATE" != ""; then
  AC_CACHE_CHECK([whether po4a-translate works],
    [ac_cv_po4a_works],
    [
      ac_cv_po4a_works=no
      $PO4A_TRANSLATE $PO4A_TRANSLATE_FLAGS -m $srcdir/doc/man/mkvmerge.xml -p $srcdir/doc/man/po4a/po/ja.po -l /dev/null > /dev/null 2> /dev/null
      if test "$?" = 0; then
        ac_cv_po4a_works=yes
      fi
    ])
  PO4A_WORKS=$ac_cv_po4a_works
fi

if test x"$PO4A_WORKS" = "xyes" ; then
  opt_features_yes="$opt_features_yes\n   * man page translations (po4a)"
else
  opt_features_no="$opt_features_no\n   * man page translations (po4a)"
fi

AC_SUBST(PO4A)
AC_SUBST(PO4A_TRANSLATE)
AC_SUBST(PO4A_FLAGS)
AC_SUBST(PO4A_TRANSLATE_FLAGS)
AC_SUBST(PO4A_WORKS)
