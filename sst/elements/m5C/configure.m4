dnl -*- Autoconf -*-

AC_DEFUN([SST_m5C_CONFIG], [

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

  AC_ARG_ENABLE([gem5-power-model], [AS_HELP_STRING([--enable-gem5-power-model], [Enable power modeling in SST-GEM5])])

  isa=

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"

  AS_IF([test ! -z "$with_gem5" -a "$with_gem5" != "yes"],
    [ CPPFLAGS="-I$with_gem5 $CPPFLAGS"
      LDFLAGS="-L$with_gem5 $LDFLAGS "],
    [])

  SST_CHECK_PYTHON([have_python=1],
	[have_python=0],
	[AC_MSG_ERROR([Python is needed for GEM5])])

  AS_IF([test ! -z "$with_gem5" -a "$with_gem5" != "no" -a "$have_python" = "no"],
	[AC_MSG_ERROR(["Cannot find Python.h, this is required for M5 component to build."])])

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([sim/system.hh], [], [sst_gem5_happy="no"])
  AC_CHECK_HEADERS([params/AlphaTLB.hh], [isa=ALPHA], [])
  AC_CHECK_HEADERS([params/SparcTLB.hh], [isa=SPARC], [])
  AC_CHECK_HEADERS([params/X86TLB.hh], [isa=X86], [])
  AC_CHECK_HEADERS([params/DerivO3CPU.hh], [use_gem5_o3=true], [use_gem5_o3=false])

  CXX_saved="$CXX"
  AS_IF([test -n "$MPICXX"], [ CXX="$MPICXX" ] )
  AC_CHECK_LIB([gem5_$with_gem5_build], [initm5], [M5_LIB="-lgem5_$with_gem5_build"], [sst_gem5_happy="no"])
  CXX="$CXX_saved"

  AC_LANG_POP(C++)

  M5_LIBDIR="$with_gem5"
  CPPFLAGS="$CPPFLAGS_saved"
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


  M5_CPPFLAGS="-I$with_gem5 -DTHE_ISA=${isa}_ISA ${cpp_extra} $M5_CPPFLAGS"
  M5_LDFLAGS="-L$with_gem5"

  AM_CONDITIONAL([USE_M5_O3], [test x$use_gem5_o3 = xtrue])

  AC_SUBST([M5_CPPFLAGS])
  AC_SUBST([M5_LDFLAGS])
  AC_SUBST([M5_LIB])
  AC_SUBST([M5_LIBDIR])

  # Now uses global DRAMSim check
  SST_CHECK_DRAMSIM([],[],[AC_MSG_ERROR([DRAMSim requested but could not be found])])

  # PHXSim configuration begin 
  AC_ARG_WITH([phxsim],
    [AS_HELP_STRING([--with-phxsim@<:@=DIR@:>@],
      [Use PHXSim package installed in optionally specified DIR])])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"
  AC_LANG_PUSH(C++)
  AS_IF([test ! -z "$with_phxsim" -a "$with_phxsim" != "yes"],
     [PHXSIM_CPPFLAGS="-I$with_phxsim" 
      CPPFLAGS="$PHXSIM_CPPFLAGS $CPPFLAGS"
      PHXSIM_LDFLAGS="-L$with_phxsim"	
      LDFLAGS="$PHXSIM_LDFLAGS $LDFLAGS"
      AC_CHECK_HEADERS([SingleCube.h], [], [sst_gem5_happy="no"])
      AC_CHECK_LIB([phxsim], [libphxsim_is_present],
                    [PHXSIM_LIB="-lphxsim"], [sst_gem5_happy="no"])
    ],
    [PHXSIM_CPPFLAGS=
     PHXSIM_LDFLAGS=]
  )

  AC_LANG_POP(C++)
  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"

  AM_CONDITIONAL([HAVE_PHXSIM],
		[test ! -z "$with_phxsim" -a "$with_phxsim" != "yes"])

  AC_SUBST([PHXSIM_CPPFLAGS])
  AC_SUBST([PHXSIM_LDFLAGS])
  AC_SUBST([PHXSIM_LIB])

  # PHXSim configuration end

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

  AS_IF([test -n "$with_gem5" -a "$with_gem5" != "no" -a "$sst_gem5_happy" = "yes"],
	[AC_DEFINE_UNQUOTED([HAVE_M5], [1], [Defines whether M5 was found at configure time])])
  AC_DEFINE_UNQUOTED([M5_LIBDIR], ["$M5_LIBDIR"], [Defines the M5 library directory])
  AS_IF([test "$sst_gem5_happy" = "yes"], [$1], [$2])
])
