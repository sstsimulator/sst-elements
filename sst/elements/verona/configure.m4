dnl -*- Autoconf -*-

AC_DEFUN([SST_verona_CONFIG], [
  AC_ARG_ENABLE([verona], [AS_HELP_STRING([--enable-verona], [Enable the Verona tracing component])],
        [], [enable_verona="no"])

  happy="yes"

  AS_IF( [test "$enable_verona" = "yes"], [happy="yes"], [happy="no"] )

  AS_IF([test "$happy" = "yes"], [$1], [$2])
])
