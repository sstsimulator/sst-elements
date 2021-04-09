
AC_DEFUN([SST_CHECK_DRAMSIM3], [
  AC_ARG_WITH([dramsim3],
    [AS_HELP_STRING([--with-dramsim3@<:@=DIR@:>@],
      [Use DRAMSim3 package installed in optionally specified DIR])])

  sst_check_dramsim3_happy="yes"
  AS_IF([test "$with_dramsim3" = "no"], [sst_check_dramsim3_happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"

  AS_IF([test ! -z "$with_dramsim3" -a "$with_dramsim3" != "yes"],
    [DRAMSIM3_CPPFLAGS="-I$with_dramsim3/src -DHAVE_DRAMSIM3"
     CPPFLAGS="$DRAMSIM3_CPPFLAGS $CPPFLAGS"
     DRAMSIM3_LDFLAGS="-L$with_dramsim3"
     DRAMSIM3_LIBDIR="$with_dramsim3"
     LDFLAGS="$DRAMSIM3_LDFLAGS $LDFLAGS"],
    [DRAMSIM3_CPPFLAGS=
     DRAMSIM3_LDFLAGS=
     DRAMSIM3_LIBDIR=])

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([dramsim3.h], [], [sst_check_dramsim3_happy="no"])
  AC_CHECK_LIB([dramsim3], [libdramsim3_is_present],
    [DRAMSIM3_LIB="-ldramsim3"], [sst_check_dramsim3_happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"

  AC_SUBST([DRAMSIM3_CPPFLAGS])
  AC_SUBST([DRAMSIM3_LDFLAGS])
  AC_SUBST([DRAMSIM3_LIB])
  AC_SUBST([DRAMSIM3_LIBDIR])
  AM_CONDITIONAL([HAVE_DRAMSIM3], [test "$sst_check_dramsim3_happy" = "yes"])
  AS_IF([test "$sst_check_dramsim3_happy" = "yes"],
        [AC_DEFINE([HAVE_DRAMSIM3], [1], [Set to 1 if DRAMSim3 was found])])
  AC_DEFINE_UNQUOTED([DRAMSIM3_LIBDIR], ["$DRAMSIM3_LIBDIR"], [Path to DRAMSim3 library])

  AS_IF([test "$sst_check_dramsim3_happy" = "yes"], [$1], [$2])
])
