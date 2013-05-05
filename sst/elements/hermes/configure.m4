dnl -*- Autoconf -*-

AC_DEFUN([SST_hermes_CONFIG], [
  AC_ARG_ENABLE([hermes], [AS_HELP_STRING([--enable-hermes], [Enable the Hermes network component])],
        [], [enable_hermes="no"])

  happy="yes"

  AM_CONDITIONAL([BUILD_HERMES], [test "$enable_hermes" = "yes" -a "$happy" = "yes"])
  AS_IF([test "$happy" = "yes"], [$1], [$2])

])
