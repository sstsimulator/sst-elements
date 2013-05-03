dnl -*- Autoconf -*-

AC_DEFUN([SST_prospero_CONFIG], [

  happy="yes"

  happy_SAVED="$happy"
  SST_CHECK_PINTOOL([have_pin=1],[have_pin=0],[AC_MSG_ERROR([PIN was requested but not found])])
  happy="$happy_SAVED"

  AS_IF([test "$happy" = "yes"], [$1], [$2])
])
