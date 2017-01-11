
AC_DEFUN([SST_CHECK_RAMULATOR], [
  AC_ARG_WITH([ramulator],
    [AS_HELP_STRING([--with-ramulator@<:@=DIR@:>@],
      [Use Ramulator library installed in optionally specified DIR])])

  sst_check_ramulator_happy="yes"
  AS_IF([test "$with_ramulator" = "no"], [sst_check_ramulator_happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"

  AS_IF([test ! -z "$with_ramulator" -a "$with_ramulator" != "yes"],
    [RAMULATOR_CPPFLAGS="-I$with_ramulator/src -DRAMULATOR -DHAVE_RAMULATOR"
     CPPFLAGS="$RAMULATOR_CPPFLAGS $CPPFLAGS"
     RAMULATOR_LDFLAGS="-L$with_ramulator"
     RAMULATOR_LIBDIR="$with_ramulator"
     LDFLAGS="$RAMULATOR_LDFLAGS $LDFLAGS"],
    [RAMULATOR_CPPFLAGS=
     RAMULATOR_LDFLAGS=
     RAMULATOR_LIBDIR=])

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([Gem5Wrapper.h], [], [sst_check_ramulator_happy="no"])
  AC_CHECK_LIB([ramulator], [libramulator_is_present],
    [RAMULATOR_LIB="-lramulator"], [sst_check_ramulator_happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"

  AC_SUBST([RAMULATOR_CPPFLAGS])
  AC_SUBST([RAMULATOR_LDFLAGS])
  AC_SUBST([RAMULATOR_LIB])
  AC_SUBST([RAMULATOR_LIBDIR])
  AM_CONDITIONAL([HAVE_RAMULATOR], [test "$sst_check_ramulator_happy" = "yes"])
  AS_IF([test "$sst_check_ramulator_happy" = "yes"],
        [AC_DEFINE([HAVE_RAMULATOR], [1], [Set to 1 if Ramulator was found])])
  AC_DEFINE_UNQUOTED([RAMULATOR_LIBDIR], ["$RAMULATOR_LIBDIR"], [Path to Ramulator library])

  AS_IF([test "$sst_check_ramulator_happy" = "yes"], [$1], [$2])
])
