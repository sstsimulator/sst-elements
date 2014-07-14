dnl -*- Autoconf -*-

AC_DEFUN([SST_prospero_CONFIG], [

  happy="yes"

  happy_SAVED="$happy"
  SST_CHECK_LIBZ()
  happy="$happy_SAVED"

  AS_IF([test "$happy" = "yes"], [$1], [$2])
])
