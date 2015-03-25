AC_ARG_WITH(tools,[AS_HELP_STRING([--with-tools],[build the tools in the src/tools sub-directory (useful mainly for development)])],
            [BUILD_TOOLS=yes],[BUILD_TOOLS=no])

AC_SUBST(BUILD_TOOLS)
