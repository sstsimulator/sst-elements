dnl -*- Autoconf -*-

AC_DEFUN([SST_gem5_CONFIG], [

  sst_gem5_happy="yes"

  AC_ARG_WITH([gem5],
    [AS_HELP_STRING([--with-gem5@<:@=DIR@:>@],
    [Use M5 package installed in optionally specified DIR])])

  AS_IF([test "$with_gem5" = "no"], [sst_gem5_happy="no"])

  AC_ARG_WITH([gem5-build],
    [AS_HELP_STRING([--with-gem5-build=TYPE],
      [Specify the type of library, Options: opt, debug (default: opt)])],
    [],
    [with_gem5_build=opt])


  SST_CHECK_PYTHON([have_python=1])
  AS_IF([test "$have_python" = "1"], [$1], [sst_gem5_happy="no"])

  dnl AX_CXX_COMPILE_STDCXX_0X()

  isa=

  CPPFLAGS_saved="$CPPFLAGS"
  CXXFLAGS_saved="$CXXFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"

  AS_IF([test ! -z "$with_gem5" -a "$with_gem5" != "yes"],
    [ CPPFLAGS="-I$with_gem5 $CPPFLAGS"
      CXXFLAGS="$SST_CXX0X_FLAGS $CXXFLAGS"
      LDFLAGS="-L$with_gem5 $LDFLAGS "],
    [])

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([sim/system.hh], [], [sst_gem5_happy="no"])
  AC_CHECK_HEADERS([params/AlphaTLB.hh], [isa=ALPHA], [])
  AC_CHECK_HEADERS([params/SparcTLB.hh], [isa=SPARC], [])
  AC_CHECK_HEADERS([params/X86TLB.hh], [isa=X86], [])
  AC_CHECK_HEADERS([params/DerivO3CPU.hh], [use_gem5_o3=true], [use_gem5_o3=false])

  AC_CHECK_LIB([gem5_$with_gem5_build], [initm5], [M5_LIB="-lgem5_$with_gem5_build"], [sst_gem5_happy="no"])

  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  CXXFLAGS="$CXXFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"

  cpp_extra=

  case "${with_gem5_build}" in
    debug) cpp_extra="-DDEBUG -DTRACING_ON=1 -DLIBTYPE=DEBUG" ;;
    opt)   cpp_extra="-DTRACING_ON=1 -DLIBTYPE=OPT" ;;
    prof)  cpp_extra="-DNDEBUG -DTRACING_ON=0 -DLIBTYPE=PROF" ;;
    fast)  cpp_extra="-DNDEBUG -DTRACING_ON=0 -DLIBTYPE=FAST" ;;
    *) sst_gem5_happy="no" ;;
  esac


  M5_CPPFLAGS="-I$with_gem5 -DTHE_ISA=${isa}_ISA ${cpp_extra} ${PYTHON_CPPFLAGS} $M5_CPPFLAGS"
  M5_LDFLAGS="-L$with_gem5 ${PYTHON_LDFLAGS}"

  AM_CONDITIONAL([USE_M5_O3], [test x$use_gem5_o3 = xtrue])

  AC_SUBST([M5_CPPFLAGS])
  AC_SUBST([M5_LDFLAGS])
  AC_SUBST([M5_LIB])

  # Now uses global DRAMSim check
  SST_CHECK_DRAMSIM([],[],[AC_MSG_ERROR([DRAMSim requested but could not be found])])


  AS_IF([test "$enable_static" = "yes" -a "$enable_shared" = "no"],
	[AC_MSG_NOTICE([GEM5 will be configured to support static object construction.])],
	[AC_MSG_NOTICE([GEM5 will use dynamic object construction.])])

  AM_CONDITIONAL([PERFORM_M5C_STATIC_OBJECT_CONSTRUCTION],
	[test "$enable_static" = "yes" -a "$enable_shared" = "no"])

  AS_IF([test -n "$with_gem5" -a "$with_gem5" != "no" -a "$sst_gem5_happy" = "no"],
	[AC_MSG_ERROR(
		[Unable to correctly determine requirements for GEM5, configure specifies to build GEM5 but cannot build successfully],
		[1], 
	 )],
	[])

  AS_IF([test "$sst_gem5_happy" = "yes"], [$1], [$2])
])
