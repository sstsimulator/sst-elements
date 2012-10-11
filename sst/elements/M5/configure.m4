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

  AC_ARG_WITH([python-inc-dir],		
	  [AS_HELP_STRING([--with-python-inc-dir[[=DIR@]]],		
		  [Expect Python headers in the specified DIR])])

  AC_ARG_ENABLE([gem5-power-model], [Enable power modeling in SST-GEM5)])

  isa=

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"

  AS_IF([test ! -z "$with_gem5" -a "$with_gem5" != "yes"],
    [ CPPFLAGS="-I$with_gem5 $CPPFLAGS"
      LDFLAGS="-L$with_gem5 $LDFLAGS "],
    [])

  FOUND_PYTHON="no"
  PYTHON_CONFIG_EXE=""
  AC_PATH_PROG([PYTHON_CONFIG_EXE], ["python-config"], ["NOTFOUND"])

  AS_IF([test -n "$with_python_inc_dir"],
	[PYTHON_CPPFLAGS="-I$with_python_inc_dir"],
	[AS_IF([test "$PYTHON_CONFIG_EXE" != "NOTFOUND"],
		[PYTHON_CPPFLAGS=`$PYTHON_CONFIG_EXE --includes`])])

  CPPFLAGS="$PYTHON_CPPFLAGS $CPPFLAGS"

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([Python.h], [FOUND_PYTHON="yes"], [FOUND_PYTHON="no"])
  AC_LANG_POP(C++)

  AS_IF([test "$FOUND_PYTHON" == "no"],
	[AC_MSG_ERROR(["Cannot find Python.h, this is required for M5 component to build."])])
  AC_SUBST([PYTHON_CPPFLAGS])
  AC_SUBST([PYTHON_CONFIG_EXE])  

#  AS_IF([test "x$with_python_includes" != "x"],
#		  [PYTHON_CPPFLAGS="-I$with_python_includes"],
#		  [AS_IF([test ! -z "$with_python" -a "$with_python" != "yes"],
#			  [PYTHON_CPPFLAGS="-I$with_python"],
#			  [PYTHON_CPPFLAGS="-DALL_FAIL"])])
#  CPPFLAGS="$PYTHON_CPPFLAGS $CPPFLAGS"	

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([sim/system.hh], [], [happy="no"])
  AC_CHECK_HEADERS([params/AlphaTLB.hh], [isa=ALPHA], [])
  AC_CHECK_HEADERS([params/SparcTLB.hh], [isa=SPARC], [])
  AC_CHECK_HEADERS([params/X86TLB.hh], [isa=X86], [])
  AC_CHECK_HEADERS([params/DerivO3CPU.hh], [use_gem5_o3=true], [use_gem5_o3=false])

  CXX_saved="$CXX"
  AS_IF([test -n "$MPICXX"], [ CXX="$MPICXX" ] )
  AC_CHECK_LIB([gem5_$with_gem5_build], [initm5], [M5_LIB="-lgem5_$with_gem5_build"], [happy="no"])
  CXX="$CXX_saved"

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


  M5_CPPFLAGS="-I$with_gem5 -DTHE_ISA=${isa}_ISA ${cpp_extra} ${M5PYTHON_CPPFLAGS} $M5_CPPFLAGS"
  M5_LDFLAGS="-L$with_gem5"

  AM_CONDITIONAL([USE_M5_O3], [test x$use_gem5_o3 = xtrue])
  AM_CONDITIONAL([USE_OSX_DYLIB], [test `uname` = "Darwin"])

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

  AS_IF([test -z "$enable-gem5-power-model" -a "$happy" = "yes"],
	[AC_MSG_NOTICE([GEM5 Power Models are enabled.])],
	[AC_MSG_NOTICE([GEM5 Power Models are disabled for this build.])]) 
  AM_CONDITIONAL([ENABLE_GEM5_POWER_MODEL], 
	[test -z "$enable-gem5-power-model" -a "$happy" = "yes"])

  AS_IF([test -n "$with_gem5" -a "$with_gem5" != "no" -a "$happy" = "no"],
	[AC_MSG_ERROR(
		[Unable to correctly determine requirements for GEM5, configure specifies to build GEM5 but cannot build successfully],
		[1], 
	 )],
	[])

  AS_IF([test "$happy" = "yes"], [$1], [$2])
])
