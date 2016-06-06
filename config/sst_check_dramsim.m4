
AC_DEFUN([SST_CHECK_DRAMSIM], [
  AC_ARG_WITH([dramsim],
    [AS_HELP_STRING([--with-dramsim@<:@=DIR@:>@],
      [Use DRAMSim package installed in optionally specified DIR])])

  sst_check_dramsim_happy="yes"
  AS_IF([test "$with_dramsim" = "no"], [sst_check_dramsim_happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"

  AS_IF([test ! -z "$with_dramsim" -a "$with_dramsim" != "yes"],
    [DRAMSIM_CPPFLAGS="-I$with_dramsim -DHAVE_DRAMSIM"
     CPPFLAGS="$DRAMSIM_CPPFLAGS $CPPFLAGS"
     DRAMSIM_LDFLAGS="-L$with_dramsim"
     DRAMSIM_LIBDIR="$with_dramsim"
     LDFLAGS="$DRAMSIM_LDFLAGS $LDFLAGS"],
    [DRAMSIM_CPPFLAGS=
     DRAMSIM_LDFLAGS=
     DRAMSIM_LIBDIR=])

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([DRAMSim.h], [], [sst_check_dramsim_happy="no"])
  AC_CHECK_LIB([dramsim], [libdramsim_is_present],
    [DRAMSIM_LIB="-ldramsim"], [sst_check_dramsim_happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"

  AC_SUBST([DRAMSIM_CPPFLAGS])
  AC_SUBST([DRAMSIM_LDFLAGS])
  AC_SUBST([DRAMSIM_LIB])
  AC_SUBST([DRAMSIM_LIBDIR])
  AM_CONDITIONAL([HAVE_DRAMSIM], [test "$sst_check_dramsim_happy" = "yes"])
  AS_IF([test "$sst_check_dramsim_happy" = "yes"],
        [AC_DEFINE([HAVE_DRAMSIM], [1], [Set to 1 if DRAMSim was found])])
  AC_DEFINE_UNQUOTED([DRAMSIM_LIBDIR], ["$DRAMSIM_LIBDIR"], [Path to DRAMSim library])

  AS_IF([test "$sst_check_dramsim_happy" = "yes"], [$1], [$2])
])
