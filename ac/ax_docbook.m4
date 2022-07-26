AC_ARG_WITH(docbook_xsl_root,
  AC_HELP_STRING([--with-docbook-xsl-root=dir],[use dir as base for DocBook XSL stylesheets instead of looking for it; must contain the "manpages/docbook.xsl" sub-directory and file]),
  [ DOCBOOK_ROOT="$with_docbook_xsl_root" ],)

if ! test -z "$DOCBOOK_ROOT"; then
  AC_MSG_CHECKING([for DocBook XSL root directory])
  AC_MSG_RESULT([using supplied $DOCBOOK_ROOT])

else
  AC_MSG_CHECKING([for DocBook XSL root directory])

  PREFIX_FOR_XSL=${MINGW_PREFIX:-/usr}
  for i in `ls -d $PREFIX_FOR_XSL/share/xml/docbook/xsl-stylesheets*-nons 2> /dev/null` \
            $PREFIX_FOR_XSL/share/xml/docbook/stylesheet/xsl/nwalsh \
            $PREFIX_FOR_XSL/share/xml/docbook/stylesheet/nwalsh \
            `ls -d $PREFIX_FOR_XSL/share/sgml/docbook/xsl-stylesheets* $PREFIX_FOR_XSL/share/xml/docbook/xsl-stylesheets* 2> /dev/null` \
    ; do
    if test -f "$i/manpages/docbook.xsl"; then
      DOCBOOK_ROOT="$i"
      break
    elif test -f "$i/current/manpages/docbook.xsl"; then
      DOCBOOK_ROOT="$i/current"
     break
    fi
  done

  if test -z "$DOCBOOK_ROOT"; then
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([DocBook XSL stylesheets are required for building.])
  else
    AC_MSG_RESULT([$DOCBOOK_ROOT])
  fi
fi

# It's just rude to go over the net to build
XSLTPROC_FLAGS="--nonet --maxdepth 10000"

AC_ARG_WITH(xsltproc,
  AC_HELP_STRING([--with-xsltproc=prog],[use prog instead of looking for xsltproc]),
  [ XSLTPROC="$with_xsltproc" ],)

if ! test -z "$XSLTPROC"; then
  AC_MSG_CHECKING(for xsltproc)
  AC_MSG_RESULT(using supplied $XSLTPROC)
else
  AC_PATH_PROG(XSLTPROC, xsltproc)
fi

if test -z "$XSLTPROC" -o ! -x "$XSLTPROC"; then
  AC_MSG_ERROR([xsltproc wasn't found])
fi

AC_CACHE_CHECK([whether xsltproc works],
  [ac_cv_xsltproc_works],
  [
    ac_cv_xsltproc_works=no
    "$XSLTPROC" $XSLTPROC_FLAGS "$DOCBOOK_ROOT/manpages/docbook.xsl" >/dev/null 2>/dev/null << END
<?xml version="1.0" encoding='ISO-8859-1'?>
<!DOCTYPE book PUBLIC "-//OASIS//DTD DocBook XML V4.5//EN" "http://www.oasis-open.org/docbook/xml/4.5/docbookx.dtd">
<book id="test">
</book>
END
    if test "$?" = 0; then
      ac_cv_xsltproc_works=yes
    fi
  ])

if test x"$ac_cv_xsltproc_works" != xyes; then
  AC_MSG_ERROR([xsltproc doesn't work with DocBook's $DOCBOOK_ROOT/manpages/docbook.xsl])
fi

AC_SUBST(DOCBOOK_ROOT)
AC_SUBST(XSLTPROC)
AC_SUBST(XSLTPROC_FLAGS)
