# -*- Autoconf -*-
#
# SYNOPSIS
#
#   SST_CHECK_METIS
#
# DESCRIPTION
#
# LICENSE
#

AC_DEFUN([SST_CHECK_METIS],
[
  sst_check_metis_happy="yes"

  AC_ARG_WITH([metis],
    [AS_HELP_STRING([--with-metis@<:@=DIR@:>@],
      [Use METIS package installed in optionally specified DIR])])

  AS_IF([test "$with_metis" = "no"], [sst_check_metis_happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"

  AS_IF([test ! -z "$with_metis" -a "$with_metis" != "yes"],
    [METIS_CPPFLAGS="-I$with_metis/include -DHAVE_METIS=1"
     CPPFLAGS="$METIS_CPPFLAGS $CPPFLAGS"
     METIS_LDFLAGS="-L$with_metis/lib"
     LDFLAGS="$METIS_LDFLAGS $LDFLAGS"
     METIS_LIBDIR="$with_metis/lib"],
    [METIS_CPPFLAGS=
     METIS_LDFLAGS=
     METIS_LIBDIR=])

  AC_CHECK_HEADERS([metis.h], [], [sst_check_metis_happy="no"])
  AC_CHECK_LIB([metis], [METIS_NodeNDP], 
    [METIS_LIB="-lmetis -lm"], [sst_check_metis_happy="no"], [-lmetis -lm])

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"

  AC_SUBST([METIS_CPPFLAGS])
  AC_SUBST([METIS_LDFLAGS])
  AC_SUBST([METIS_LIB])
  AC_SUBST([METIS_LIBDIR])
  AM_CONDITIONAL([HAVE_METIS], [test "$sst_check_metis_happy" = "yes"])
  AS_IF([test "$sst_check_metis_happy" = "yes"],
	[AC_DEFINE([HAVE_METIS], [1], [Set to 1 if METIS is located successfully])])
  AC_DEFINE_UNQUOTED([METIS_LIBDIR], ["$METIS_LIBDIR"], [Path to the METIS library])

  AC_MSG_CHECKING([for METIS package])
  AC_MSG_RESULT([$sst_check_metis_happy])
  AS_IF([test "$sst_check_metis_happy" = "yes"], [$1], [$2])
])
