
AC_DEFUN([SST_CHECK_HYBRIDSIM], [
  AC_ARG_WITH([hybridsim],
    [AS_HELP_STRING([--with-hybridsim@<:@=DIR@:>@],
      [Use HybridSim package installed in optionally specified DIR])])

  sst_check_hybridsim_happy="yes"
  AS_IF([test "$with_hybridsim" = "no"], [sst_check_hybridsim_happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"

  AS_IF([test ! -z "$with_hybridsim" -a "$with_hybridsim" != "yes"],
    [HYBRIDSIM_CPPFLAGS="-I$with_hybridsim"
     CPPFLAGS="$HYBRIDSIM_CPPFLAGS $CPPFLAGS"
     HYBRIDSIM_LDFLAGS="-L$with_hybridsim"
     LDFLAGS="$HYBRIDSIM_LDFLAGS $LDFLAGS"
     HYBRIDSIM_LIBDIR="$with_hybridsim"],
    [HYRBRIDSIM_CPPFLAGS=
     HYBRIDSIM_LDFLAGS=
     HYBRIDSIM_LIBDIR=])

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([HybridSim.h], [], [sst_check_hybridsim_happy="no"])
  AC_CHECK_LIB([hybridsim], [libhybridsim_is_present],
    [HYBRIDSIM_LIB="-lhybridsim"], [sst_check_hybridsim_happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"

  AC_SUBST([HYBRIDSIM_CPPFLAGS])
  AC_SUBST([HYBRIDSIM_LDFLAGS])
  AC_SUBST([HYBRIDSIM_LIB])
  AC_SUBST([HYBRIDSIM_LIBDIR])
  AM_CONDITIONAL([HAVE_HYBRIDSIM], [test "$sst_check_hybridsim_happy" = "yes"])
  AS_IF([test "$sst_check_hybridsim_happy" = "yes"],
        [AC_DEFINE([HAVE_HYBRIDSIM], [1], [Set to 1 if HybridSim was found])])
  AC_DEFINE_UNQUOTED([HYBRIDSIM_LIBDIR], ["$HYBRIDSIM_LIBDIR"], [Path to HybridSim library])

  AS_IF([test "$sst_check_hybridsim_happy" = "yes"], [$1], [$2])
])
