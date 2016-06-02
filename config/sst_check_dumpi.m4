AC_DEFUN([SST_CHECK_DUMPI],
[
  sst_check_dumpi_happy="yes"

  AC_ARG_WITH([dumpi],
    [AS_HELP_STRING([--with-dumpi@<:@=DIR@:>@],
      [Use SST DUMPI tracing tools installed in optionally specified DIR])])

  AS_IF([test "$with_dumpi" = "no"], [sst_check_dumpi_happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"

  AS_IF([test "$sst_check_dumpi_happy" = "yes"], [
    AS_IF([test ! -z "$with_dumpi" -a "$with_dumpi" != "yes"],
      [DUMPI_CPPFLAGS="-I$with_dumpi/include"
       CPPFLAGS="$DUMPI_CPPFLAGS $CPPFLAGS"
       DUMPI_LDFLAGS="-L$with_dumpi/lib"
       DUMPI_LIB="-lundumpi",
       LDFLAGS="$DUMPI_LDFLAGS $LDFLAGS"],
      [DUMPI_CPPFLAGS=
       DUMPI_LDFLAGS=
       DUMPI_LIB=])])
  
  AC_LANG_PUSH([C++])
  AC_CHECK_HEADER([dumpi/libundumpi/libundumpi.h], [], [sst_check_dumpi_happy="no"])
  AC_LANG_POP([C++])

  AC_CHECK_LIB([undumpi], [undumpi_open],
    [DUMPI_LIB="-lundumpi"], [sst_check_dumpi_happy="no"])

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"

  AC_SUBST([DUMPI_CPPFLAGS])
  AC_SUBST([DUMPI_LDFLAGS])
  AC_SUBST([DUMPI_LIB])
  AS_IF([test "x$sst_check_dumpi_happy" = "xyes"], [AC_DEFINE([HAVE_DUMPI],[1],[Defines whether we have the DUMPI library])])
  AM_CONDITIONAL([USE_DUMPI], [test "x$sst_check_dumpi_happy" = "xyes"])

  AC_MSG_CHECKING([for DUMPI tracing library])
  AC_MSG_RESULT([$sst_check_dumpi_happy])
  AS_IF([test "$sst_check_dumpi_happy" = "no" -a ! -z "$with_dumpi" -a "$with_dumpi" != "no"], [$3])
  AS_IF([test "$sst_check_dumpi_happy" = "yes"], [$1], [$2])
])
