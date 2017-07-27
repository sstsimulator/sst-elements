
AC_DEFUN([SST_CHECK_HBMDRAMSIM], [
  AC_ARG_WITH([hbmdramsim],
    [AS_HELP_STRING([--with-hbmdramsim@<:@=DIR@:>@],
      [Use HBM DRAMSim package installed in optionally specified DIR])])

  sst_check_hbmdramsim_happy="yes"
  AS_IF([test "$with_hbmdramsim" = "no"], [sst_check_hbmdramsim_happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"

  AS_IF([test ! -z "$with_hbmdramsim" -a "$with_hbmdramsim" != "yes"],
    [HBMDRAMSIM_CPPFLAGS="-I$with_hbmdramsim -DHAVE_HBMDRAMSIM -DHAVE_HBMLIBDRAMSIM"
     CPPFLAGS="$HBMDRAMSIM_CPPFLAGS $CPPFLAGS"
     HBMDRAMSIM_LDFLAGS="-L$with_hbmdramsim"
     HBMDRAMSIM_LIBDIR="$with_hbmdramsim"
     LDFLAGS="$HBMDRAMSIM_LDFLAGS $LDFLAGS"],
    [HBMDRAMSIM_CPPFLAGS=
     HBMDRAMSIM_LDFLAGS=
     HBMDRAMSIM_LIBDIR=])

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([HBMDRAMSim.h], [], [sst_check_hbmdramsim_happy="no"])
  AC_CHECK_LIB([hbmdramsim], [libdramsim_is_present],
    [HBMDRAMSIM_LIB="-ldramsim"], [sst_check_hbmdramsim_happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"

  AC_SUBST([HBMDRAMSIM_CPPFLAGS])
  AC_SUBST([HBMDRAMSIM_LDFLAGS])
  AC_SUBST([HBMDRAMSIM_LIB])
  AC_SUBST([HBMDRAMSIM_LIBDIR])
  AM_CONDITIONAL([HAVE_HBMDRAMSIM], [test "$sst_check_hbmdramsim_happy" = "yes"])
  AS_IF([test "$sst_check_hbmdramsim_happy" = "yes"],
        [AC_DEFINE([HAVE_HBMDRAMSIM], [1], [Set to 1 if HBM DRAMSim was found])])
  AC_DEFINE_UNQUOTED([HBMDRAMSIM_LIBDIR], ["$HBMDRAMSIM_LIBDIR"], [Path to HBM DRAMSim library])

  AS_IF([test "$sst_check_hbmdramsim_happy" = "yes"], [$1], [$2])
])
