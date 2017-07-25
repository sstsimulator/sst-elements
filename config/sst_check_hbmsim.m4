
AC_DEFUN([SST_CHECK_HBMSIM], [
  AC_ARG_WITH([hbmsim],
    [AS_HELP_STRING([--with-hbmsim@<:@=DIR@:>@],
      [Use HBMSim package installed in optionally specified DIR])])

  sst_check_hbmsim_happy="yes"
  AS_IF([test "$with_hbmsim" = "no"], [sst_check_hbmsim_happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"

  AS_IF([test ! -z "$with_hbmsim" -a "$with_hbmsim" != "yes"],
    [HBMSIM_CPPFLAGS="-I$with_hbmsim -DHAVE_HBMSIM"
     CPPFLAGS="$HBMSIM_CPPFLAGS $CPPFLAGS"
     HBMSIM_LDFLAGS="-L$with_hbmsim"
     HBMSIM_LIBDIR="$with_hbmsim"
     LDFLAGS="$HBMSIM_LDFLAGS $LDFLAGS"],
    [HBMSIM_CPPFLAGS=
     HBMSIM_LDFLAGS=
     HBMSIM_LIBDIR=])

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([HBMSim.h], [], [sst_check_hbmsim_happy="no"])
  AC_CHECK_LIB([hbmsim], [libhbmsim_is_present],
    [HBMSIM_LIB="-lhbmsim"], [sst_check_hbmsim_happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"

  AC_SUBST([HBMSIM_CPPFLAGS])
  AC_SUBST([HBMSIM_LDFLAGS])
  AC_SUBST([HBMSIM_LIB])
  AC_SUBST([HBMSIM_LIBDIR])
  AM_CONDITIONAL([HAVE_HBMSIM], [test "$sst_check_hbmsim_happy" = "yes"])
  AS_IF([test "$sst_check_hbmsim_happy" = "yes"],
        [AC_DEFINE([HAVE_HBMSIM], [1], [Set to 1 if HBMSim was found])])
  AC_DEFINE_UNQUOTED([HBMSIM_LIBDIR], ["$HBMSIM_LIBDIR"], [Path to HBMSim library])

  AS_IF([test "$sst_check_hbmsim_happy" = "yes"], [$1], [$2])
])
