dnl -*- Autoconf -*-

AC_DEFUN([SST_prospero_CONFIG], [
  prospero_happy="yes"

  SST_CHECK_LIBZ()

  AS_IF([test "$prospero_happy" = "yes"], [$1], [$2])
])
