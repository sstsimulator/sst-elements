dnl -*- Autoconf -*-

AC_DEFUN([SST_M5_CONFIG], [
  AC_ARG_WITH([m5],
    [AS_HELP_STRING([--with-m5@<:@=DIR@:>@],
      [Use M5 package installed in optionally specified DIR])])

  AC_ARG_ENABLE([m5-isa],
    [AS_HELP_STRING([--enable-m5-isa=ISA],
      [ALPHA_ISA, SPARC_ISA])])

  AS_IF([test "$with_m5" = "no"], [happy="no"])
  AS_IF([test "$enable_m5_isa" = "no"], [happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"

  AS_IF([test ! -z "$with_m5" -a "$with_m5" != "yes"],
    [M5_CPPFLAGS="-I$with_m5 \
        -DTHE_ISA=$enable_m5_isa \
        -DTRACING_ON=1 \
        -DDEBUG"
     CPPFLAGS="$M5_CPPFLAGS $CPPFLAGS"
     M5_LDFLAGS="-L$with_m5"
     LDFLAGS="$M5_LDFLAGS $LDFLAGS"],
    [M5_CPPFLAGS=
     M5_LDFLAGS=])

  use_m5_o3=false 
  AS_IF([test "$enable_m5_isa" != "no"], 
    [case "$enable_m5_isa" in
      ALPHA_ISA | SPARC_ISA) use_m5_o3=true ;;
     esac])

  AM_CONDITIONAL([USE_M5_O3], [test x$use_m5_o3 = xtrue])

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([sim/system.hh], [], [happy="no"])
  AC_CHECK_LIB([m5], [initm5],
    [M5_LIB="-lm5"], [happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"

  AC_SUBST([M5_CPPFLAGS])
  AC_SUBST([M5_LDFLAGS])
  AC_SUBST([M5_LIB])

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"

  AS_IF([test ! -z "$with_dramsim" -a "$with_dramsim" != "yes"],
    [DRAMSIM_CPPFLAGS="-I$with_dramsim"
     CPPFLAGS="$DRAMSIM_CPPFLAGS $CPPFLAGS"
     DRAMSIM_LDFLAGS="-L$with_dramsim"
     LDFLAGS="$DRAMSIM_LDFLAGS $LDFLAGS"],
    [DRAMSIM_CPPFLAGS=
     DRAMSIM_LDFLAGS=])

  AM_CONDITIONAL([HAVE_DRAMSIM], 
		[test ! -z "$with_dramsim" -a "$with_dramsim" != "yes"])

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([MemorySystem.h], [], [happy="no"])
  AC_CHECK_LIB([dramsim], [libdramsim_is_present],
    [DRAMSIM_LIB="-ldramsim"], [happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"

  AC_SUBST([DRAMSIM_CPPFLAGS])
  AC_SUBST([DRAMSIM_LDFLAGS])
  AC_SUBST([DRAMSIM_LIB])

  AS_IF([test "$happy" = "yes"], [$1], [$2])
])
