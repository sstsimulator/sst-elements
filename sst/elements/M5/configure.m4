dnl -*- Autoconf -*-

AC_DEFUN([SST_M5_CONFIG], [

  happy="yes"

  AC_ARG_WITH([gem5],
    [AS_HELP_STRING([--with-gem5@<:@=DIR@:>@],
    [Use M5 package installed in optionally specified DIR])])

  AS_IF([test "$with_gem5" = "no"], [happy="no"])

  AC_ARG_WITH([gem5-build],
    [AS_HELP_STRING([--with-gem5-build=TYPE],
      [Specify the type of library, Options: opt, debug (default: opt)])],
    [],
    [with_gem5_build=opt])

  AC_ARG_WITH([python],
    [AS_HELP_STRING([--with-python@<:@=DIR@:>@],
    [Use Python installed in optionally specified DIR])])

  AC_ARG_ENABLE([gem5-power-model], [Enable power modeling in SST-GEM5)])

  isa=

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"

  AS_IF([test ! -z "$with_gem5" -a "$with_gem5" != "yes"],
    [ CPPFLAGS="-I$with_gem5 $CPPFLAGS"
      LDFLAGS="-L$with_gem5 $LDFLAGS "],
    [])

  AS_IF([test ! -z "$with_python" -a "$with_python" != "yes"],
    [ CPPFLAGS="-I$with_python $CPPFLAGS"],
    [])

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([Python.h], [M5_CPPFLAGS="-I$with_python $CPPFLAGS"], [happy="no"])
  AC_CHECK_HEADERS([sim/system.hh], [], [happy="no"])
  AC_CHECK_HEADERS([params/AlphaTLB.hh], [isa=ALPHA], [])
  AC_CHECK_HEADERS([params/SparcTLB.hh], [isa=SPARC], [])
  AC_CHECK_HEADERS([params/X86TLB.hh], [isa=X86], [])
  AC_CHECK_HEADERS([params/DerivO3CPU.hh], [use_gem5_o3=true], [use_gem5_o3=false])
  AC_CHECK_LIB([gem5_$with_gem5_build], [initm5], [M5_LIB="-lgem5_$with_gem5_build"], [happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"

  cpp_extra=

  case "${with_gem5_build}" in
    debug) cpp_extra="-DDEBUG -DTRACING_ON=1 -DLIBTYPE=DEBUG" ;;
    opt)   cpp_extra="-DTRACING_ON=1 -DLIBTYPE=OPT" ;;
    prof)  cpp_extra="-DNDEBUG -DTRACING_ON=0 -DLIBTYPE=PROF" ;;
    fast)  cpp_extra="-DNDEBUG -DTRACING_ON=0 -DLIBTYPE=FAST" ;;
    *) happy="no" ;;
  esac

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([Python.h], [M5PYTHON_CPPFLAGS="-I$with_python $CPPFLAGS"], [happy="no"])
  AC_LANG_POP(C++)

  M5_CPPFLAGS="-I$with_gem5 -DTHE_ISA=${isa}_ISA ${cpp_extra} ${M5PYTHON_CPPFLAGS}"
  M5_LDFLAGS="-L$with_gem5"

  AM_CONDITIONAL([USE_M5_O3], [test x$use_gem5_o3 = xtrue])

  AC_SUBST([M5_CPPFLAGS])
  AC_SUBST([M5_LDFLAGS])
  AC_SUBST([M5_LIB])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"
  AC_LANG_PUSH(C++)

  AS_IF([test ! -z "$with_dramsim" -a "$with_dramsim" != "yes"],
    [
      DRAMSIM_CPPFLAGS="-I$with_dramsim" 
      DRAMSIM_LDFLAGS="-L$with_dramsim"	
      AC_CHECK_HEADERS([MemorySystem.h], [], [happy="no"])
      AC_CHECK_LIB([dramsim], [libdramsim_is_present], [], [happy="no"])
    ],
    [DRAMSIM_CPPFLAGS=
     DRAMSIM_LDFLAGS=]
  )

  AC_LANG_POP(C++)
  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"

  AM_CONDITIONAL([HAVE_DRAMSIM],
		[test ! -z "$with_dramsim" -a "$with_dramsim" != "yes"])

  AC_SUBST([DRAMSIM_CPPFLAGS])
  AC_SUBST([DRAMSIM_LDFLAGS])
  AC_SUBST([DRAMSIM_LIB])

  AS_IF([test -z "$enable-gem5-power-model" -a "happy" = "yes"],
	[AC_MSG_NOTICE([GEM5 Power Models are enabled.])],
	[AC_MSG_NOTICE([GEM5 Power Models are disabled for this build.])]) 
  AM_CONDITIONAL([ENABLE_GEM5_POWER_MODEL], 
	[test -z "$enable-gem5-power-model" -a "$happy" = "yes"])

  AS_IF([test "$happy" = "yes"], [$1], [$2])
])
