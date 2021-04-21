
AC_DEFUN([SST_CHECK_SWM], [
  AC_ARG_WITH([swm],
    [AS_HELP_STRING([--with-swm@<:@=DIR@:>@],
      [Use SWM package installed in optionally specified DIR])])

  sst_check_swm_happy="yes"
  AS_IF([test "$with_swm" = "no"], [sst_check_swm_happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"

  AS_IF([test ! -z "$with_swm" -a "$with_swm" != "yes"],
    [SWM_CPPFLAGS="-I$with_swm/include -DHAVE_SWM"
     CPPFLAGS="$SWM_CPPFLAGS $CPPFLAGS"
     SWM_LDFLAGS="-L$with_swm/lib"
     SWM_LIBDIR="$with_swm/lib"
	 SWM_LIB="-lswm"
     LDFLAGS="$SWM_LDFLAGS $LDFLAGS"],
    [SWM_CPPFLAGS=
     SWM_LDFLAGS=
     SWM_LIBDIR=])

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([swm-include.h], [], [sst_check_swm_happy="no"])
  #AC_CHECK_LIB([swm], [libswm_is_present],
  #  [SWM_LIB="-lswm"], [sst_check_swm_happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"

  AC_SUBST([SWM_CPPFLAGS])
  AC_SUBST([SWM_LDFLAGS])
  AC_SUBST([SWM_LIB])
  AC_SUBST([SWM_LIBDIR])
  AM_CONDITIONAL([HAVE_SWM], [test "$sst_check_swm_happy" = "yes"])
  AS_IF([test "$sst_check_swm_happy" = "yes"],
        [AC_DEFINE([HAVE_SWM], [1], [Set to 1 if SWM was found])])
  AC_DEFINE_UNQUOTED([SWM_LIBDIR], ["$SWM_LIBDIR"], [Path to SWM library])

  AS_IF([test "$sst_check_swm_happy" = "yes"], [$1], [$2])
])
