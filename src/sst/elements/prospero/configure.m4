dnl -*- Autoconf -*-

AC_DEFUN([SST_prospero_CONFIG], [

  prospero_happy="yes"

  SST_CHECK_LIBZ()
  SST_CHECK_PINTOOL([have_pin=1],[have_pin=0],[])

  AS_IF([test "$prospero_happy" = "yes"], [$1], [$2])
])
