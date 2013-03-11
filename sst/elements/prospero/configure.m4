dnl -*- Autoconf -*-

AC_DEFUN([SST_prospero_CONFIG], [
  AC_ARG_ENABLE([prospero], [AS_HELP_STRING([--enable-prospero], [Enable the Prospero tracing component])],
        [], [enable_prospero="no"])

  happy="yes"

  AS_IF( [test "$enable_prospero" = "yes"], [happy="yes"], [happy="no"] )

  AS_IF([test "$happy" = "yes"], [$1], [$2])
])
