dnl -*- Autoconf -*-

AC_DEFUN([SST_M5_CONFIG], [
  AC_ARG_WITH([m5],
    [AS_HELP_STRING([--with-m5@<:@=DIR@:>@],
    [Use M5 package installed in optionally specified DIR])])

  AS_IF([test "$with_m5" = "no"], [happy="no"])

  AC_ARG_WITH([m5-build],
    [AS_HELP_STRING([--with-m5-build=TYPE],
      [Specify the type of library, Options: opt, debug (default: opt)])],
    [],
    [with_m5_build=opt])

  isa=

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"

  AS_IF([test ! -z "$with_m5" -a "$with_m5" != "yes"],
    [ CPPFLAGS="-I$with_m5 $CPPFLAGS"
      LDFLAGS="-L$with_m5 $LDFLAGS"],
    [])

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([sim/system.hh], [], [happy="no"])
  AC_CHECK_HEADERS([params/AlphaTLB.hh], [isa=ALPHA], [])
  AC_CHECK_HEADERS([params/SparcTLB.hh], [isa=SPARC], [])
  AC_CHECK_HEADERS([params/X86TLB.hh], [isa=X86], [])
  AC_CHECK_HEADERS([params/DerivO3CPU.hh], [use_m5_o3=true], [use_m5_o3=false])
  AC_CHECK_LIB([m5_$with_m5_build], [initm5], [], [happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"

  cpp_extra=

  case "${with_m5_build}" in
    debug) cpp_extra="-DDEBUG -DTRACING_ON=1" ;;
    opt)   cpp_extra="-DTRACING_ON=1" ;;
    prof)  cpp_extra="-DNDEBUG -DTRACING_ON=0" ;;
    fast)  cpp_extra="-DNDEBUG -DTRACING_ON=0" ;;
    *) happy="no" ;;
  esac

  M5_CPPFLAGS="-I$with_m5 -DTHE_ISA=${isa}_ISA ${cpp_extra}"
  M5_LDFLAGS="-L$with_m5"

  AM_CONDITIONAL([USE_M5_O3], [test x$use_m5_o3 = xtrue])

  AC_SUBST([M5_CPPFLAGS])
  AC_SUBST([M5_LDFLAGS])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"

  AS_IF([test ! -z "$with_dramsim" -a "$with_dramsim" != "yes"],
    [DRAMSIM_CPPFLAGS="-I$with_dramsim"
     CPPFLAGS="$DRAMSIM_CPPFLAGS $CPPFLAGS"
     DRAMSIM_LDFLAGS="-L$with_dramsim"
     LDFLAGS="$DRAMSIM_LDFLAGS $LDFLAGS"],
    [DRAMSIM_CPPFLAGS=
     DRAMSIM_LDFLAGS=])

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([MemorySystem.h], [], [happy="no"])
  AC_CHECK_LIB([dramsim], [libdramsim_is_present],
    [DRAMSIM_LIB="-ldramsim"], [happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"

  AM_CONDITIONAL([HAVE_DRAMSIM], 
		[test ! -z "$with_dramsim" -a "$with_dramsim" != "yes"])

  AC_SUBST([DRAMSIM_CPPFLAGS])
  AC_SUBST([DRAMSIM_LDFLAGS])

  AS_IF([test "$happy" = "yes"], [$1], [$2])
])
