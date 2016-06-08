dnl -*- Autoconf -*-

AC_DEFUN([SST_ariel_CONFIG], [
  sst_check_ariel="yes"

  SST_CHECK_PINTOOL([have_pin=1],[have_pin=0],[AC_MSG_ERROR([PIN was requested but not found])])

  AS_IF( [test "$have_pin" = 1], [sst_check_ariel="yes"], [sst_check_ariel="no"] )
  AS_IF([test "$sst_check_ariel" = "yes"], [$1], [$2])
])
