
AC_DEFUN([SST_cpu_PowerAndData_CONFIG], [
  AC_ARG_WITH([sim-panalyzer],
    [AS_HELP_STRING([--with-panalyzer@<:@=DIR@:>@],
      [Use Sim-Panalyzer package installed in optionally specified DIR])])

  AS_IF([test "$with_panalyzer" = "no"], [happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"

  AS_IF([test ! -z "$with_panalyzer" -a "$with_panalyzer" != "yes"],
    [SIMPANALYZER_CPPFLAGS="-I$with_panalyzer"
     CPPFLAGS="$SIMPANALYZER_CPPFLAGS $CPPFLAGS"
     SIMPANALYZER_LDFLAGS="-L$with_panalyzer"
     LDFLAGS="$SIMPANALYZER_LDFLAGS $LDFLAGS"],
    [SIMPANALYZER_CPPFLAGS=
     SIMPANALYZER_LDFLAGS=])

  AC_LANG_PUSH(C++)  
  AC_CHECK_LIB([sim-panalyzer], [lv1_panalyzer], 
    [SIMPANALYZER_LIB="-lsim-panalyzer"], [happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"

  AC_SUBST([SIMPANALYZER_CPPFLAGS])
  AC_SUBST([SIMPANALYZER_LDFLAGS])
  AC_SUBST([SIMPANALYZER_LIB])

  AS_IF([test "$happy" = "yes"], [$1], [$2])

])
