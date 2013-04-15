dnl -*- Autoconf -*-

AC_DEFUN([SST_ariel_CONFIG], [
  AC_ARG_ENABLE([ariel], [AS_HELP_STRING([--enable-ariel], [Enable the Ariel tracing component])],
        [], [enable_ariel="no"])

  happy="yes"

  AS_IF( [test "$enable_ariel" = "yes"], [happy="yes"], [happy="no"] )

  AS_IF([test "$happy" = "yes"], [$1], [$2])
])
