dnl -*- Autoconf -*-

AC_DEFUN([SST_zodiac_CONFIG], [
  SST_CHECK_DUMPI([have_dumpilib=1],
              [have_dumpilib=0],
              [AC_MSG_ERROR([SST DUMPI requested but not found])])

  AS_IF([test "$have_dumpilib" = 1],
   [AC_DEFINE([have_dumpilib], [1], [Define if you have the SST DUMPI library.])])

  AS_IF([test "$have_dumpilib" = "1"], [$1], [$2])
])
