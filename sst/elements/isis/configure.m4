dnl -*- Autoconf -*-

AC_DEFUN([SST_isis_CONFIG], [
  AC_ARG_ENABLE([isis], [AS_HELP_STRING([--enable-isis], [Enable the Isis MPI trace component])],
        [], [enable_isis="no"])

  AS_IF( [ test -z "$test_isis" -a "$enable_isis" = "yes" ], [ happy = "yes" ] )
  AS_IF([test "$happy" = "yes"], [$1], [$2])
])
