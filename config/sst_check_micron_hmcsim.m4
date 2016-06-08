AC_DEFUN([SST_CHECK_MICRON_HMCSIM], [

  sst_check_micron_hmcsim_happy="yes"

  AC_ARG_WITH([micron-hmcsim],
    [AS_HELP_STRING([--with-micron-hmcsim@<:@=DIR@:>@],
    [Use Micron's HMC Sim package installed in optionally specified DIR])])

  SST_CHECK_SYSTEMC([], [sst_check_micron_hmcsim_happy="no"])
#  AX_CXX_COMPILE_STDCXX_0X([], [sst_check_micron_hmcsim_happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"

  AC_MSG_NOTICE([HMC checks will use C++11 flags ($SST_CXX0X_FLAGS)])

  AS_IF([test ! -z "$with_micron_hmcsim" -a "$with_micron_hmcsim" != "yes"],
    [ 	LDFLAGS="-L$with_micron_hmcsim $LDFLAGS $SST_SYSTEMC_LDFLAGS"
        CPPFLAGS="-I$with_micron_hmcsim $SST_SYSTEMC_CPPFLAGS $CPPFLAGS $SST_CXX0X_FLAGS"
	MICRON_HMCSIM_LIBDIR="$with_micron_hmcsim" 
	LIBS="-lhmcsim $SST_SYSTEMC_LIB $LIBS "],
    [])

  MICRON_HMCSIM_CPPFLAGS="$CPPFLAGS $SST_SYSTEMC_CPPFLAGS"
  MICRON_HMCSIM_LDFLAGS="$LDFLAGS $SST_SYSTEMC_LDFLAGS -L$with_micron_hmcsim"
  MICRON_HMCSIM_LIBS="-lhmcsim $SST_SYSTEMC_LIBS"

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([HMCSim.h], [], [sst_check_micron_hmcsim_happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"

  AC_SUBST([MICRON_HMCSIM_LIBDIR])
  AC_SUBST([MICRON_HMCSIM_CPPFLAGS])
  AC_SUBST([MICRON_HMCSIM_LIBS])
  AC_SUBST([MICRON_HMCSIM_LDFLAGS])

  AS_IF([test "x$sst_check_micron_hmcsim_happy" = "xyes"], 
	[AC_DEFINE_UNQUOTED([MICRON_HMCSIM_LIBDIR], ["$MICRON_HMCSIM_LIBDIR"],
	[Path to the Micron HMCSim library]) ])
  AS_IF([test "x$sst_check_micron_hmcsim_happy" = "xyes"], 
	[AC_DEFINE([HAVE_MICRON_HMCSIM], [1],
	[Defines if we have the Micron HMCSim component]) ])

  AS_IF([test "$sst_check_micron_hmcsim_happy" = "yes"], [$1], [$2])
])
