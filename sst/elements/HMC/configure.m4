
AC_DEFUN([SST_HMC_CONFIG], [
  sst_micron_hmcsim_happy="yes"

  SST_CHECK_SYSTEMC([sst_micron_hmcsim_found_systemc_happy="yes"],
	[sst_micron_hmcsim_found_systemc_happy="no"])

  AS_IF([test "x$sst_micron_hmcsim_found_systemc_happy" = "xno"],
	[AC_MSG_ERROR([SystemC header files count not be found for Micron HMCSim], [1])])

  AC_ARG_WITH([micron-hmcsim],
    [AS_HELP_STRING([--with-micron-hmcsim@<:@=DIR@:>@],
    [Use Micron's HMC Sim package installed in optionally specified DIR])])

  AS_IF([test "$with_micron_hmcsim" = "no"], [sst_micron_hmcsim_happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"

  AS_IF([test ! -z "$with_micron_hmcsim" -a "$with_micron_hmcsim" != "yes"],
    [ 	LDFLAGS="-L$with_micron_hmcsim $LDFLAGS $SST_SYSTEMC_LDFLAGS"
        CPPFLAGS="-I$with_micron_hmcsim $SST_SYSTEMC_CPPFLAGS $CPPFLAGS"
	MICRON_HMCSIM_LIBDIR="$with_micron_hmcsim" 
	LIBS="-lhmcsim $SST_SYSTEMC_LIB $LIBS "],
    [])

  MICRON_HMCSIM_CPPFLAGS="$CPPFLAGS $SST_SYSTEMC_CPPFLAGS"
  MICRON_HMCSIM_LDFLAGS="$LDFLAGS $SST_SYSTEMC_LDFLAGS -L$with_micron_hmcsim"
  MICRON_HMCSIM_LIBS="-lhmcsim $SST_SYSTEMC_LIBS"

#  AC_LANG_PUSH(C++)
#  AC_CHECK_LIB([hmcsim], [sc_main],
#	[MICRON_HMCSIM_LIB="-lhmcsim"], 
#	[sst_micron_hmcsim_happy="no"])
#  AC_LANG_POP(C++)

  AC_CHECK_HEADERS([HMCSim.h], [], [sst_micron_hmcsim_found_systemc_happy="no"])

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"

  AC_SUBST([MICRON_HMCSIM_LIBDIR])
  AC_SUBST([MICRON_HMCSIM_CPPFLAGS])
  AC_SUBST([MICRON_HMCSIM_LIBS])
  AC_SUBST([MICRON_HMCSIM_LDFLAGS])

  AS_IF([test "$sst_micron_hmcsim_happy" = "yes"], [$1], [$2])
])
