TRANSLATE_PERCENT=0
PO4A_FLAGS="-k $TRANSLATE_PERCENT"

AC_ARG_WITH(po4a,
  AS_HELP_STRING([--with-po4a=prog],[use prog instead of looking for po4a]),
  [ PO4A="$with_po4a" ],)

if ! test -z "$PO4A"; then
  AC_MSG_CHECKING(for po4a)
  AC_MSG_RESULT(using supplied $PO4A)
else
  AC_PATH_PROG(PO4A, po4a)
fi

if test x"$PO4A" != x ; then
  opt_features_yes="$opt_features_yes\n   * man page translations (po4a)"
else
  opt_features_no="$opt_features_no\n   * man page translations (po4a)"
fi

AC_SUBST(PO4A)
AC_SUBST(PO4A_FLAGS)
