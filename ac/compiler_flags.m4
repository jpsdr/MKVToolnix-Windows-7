QUNUSED_ARGUMENTS=""
WNO_SELF_ASSIGN=""
WNO_MISMATCHED_TAGS=""
WLOGICAL_OP=""

if test $COMPILER_TYPE = clang; then
  QUNUSED_ARGUMENTS="-Qunused-arguments"
  WNO_SELF_ASSIGN="-Wno-self-assign"
  WNO_MISMATCHED_TAGS="-Wno-mismatched-tags"
  FSTACK_PROTECTOR=""

  ac_save_CXXFLAGS="$CXXFLAGS"
  AC_LANG_PUSH(C++)

  CXXFLAGS="$ac_save_CXXFLAGS -Werror -Wno-inconsistent-missing-override"
  AC_TRY_COMPILE([],[],[ WNO_INCONSISTENT_MISSING_OVERRIDE="-Wno-inconsistent-missing-override" ],[])

  CXXFLAGS="$ac_save_CXXFLAGS -Werror -Wno-potentially-evaluated-expression"
  AC_TRY_COMPILE([],[],[ WNO_POTENTIALLY_EVALUATED_EXPRESSION="-Wno-potentially-evaluated-expression" ],[])

  if check_version 3.5.0 $COMPILER_VERSION ; then
    FSTACK_PROTECTOR="-fstack-protector-strong"
  fi

  AC_LANG_POP()
  CXXFLAGS="$ac_save_CXXFLAGS"

  AC_PATH_PROG(LLVM_LLD, "lld")

else
  WNO_MAYBE_UNINITIALIZED="-Wno-maybe-uninitialized"
  FSTACK_PROTECTOR="-fstack-protector"

  if check_version 4.9.0 $COMPILER_VERSION ; then
    FSTACK_PROTECTOR="-fstack-protector-strong"

  elif check_version 4.8.0 $COMPILER_VERSION ; then
    WLOGICAL_OP="-Wlogical-op"
  fi
fi

AC_SUBST(FSTACK_PROTECTOR)
AC_SUBST(QUNUSED_ARGUMENTS)
AC_SUBST(WNO_SELF_ASSIGN)
AC_SUBST(WNO_MISMATCHED_TAGS)
AC_SUBST(WLOGICAL_OP)
AC_SUBST(WNO_INCONSISTENT_MISSING_OVERRIDE)
AC_SUBST(WNO_POTENTIALLY_EVALUATED_EXPRESSION)
AC_SUBST(WNO_MAYBE_UNINITIALIZED)
AC_SUBST(LLVM_LLD)
