dnl -*- Autoconf -*-

AC_DEFUN([SST_isis_CONFIG], [
  SST_CHECK_DUMPI([isis_have_dumpi=1],
              [isis_have_dumpi=0],
              [AC_MSG_ERROR([DUMPI requested but not found])])
  AS_IF([test "$isis_have_dumpi" = "1"], [AC_DEFINE([HAVE_DUMPI], [1], [Define if DUMPI can be used for builds])])

  AS_IF([test "$isis_have_dumpi" = "1"], [$1], [$2])
])
