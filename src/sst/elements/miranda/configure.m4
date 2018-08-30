dnl -*- Autoconf -*-
dnl vim:ft=config
dnl

AC_DEFUN([SST_miranda_CONFIG], [
	miranda_happy="yes"

  # Use global Stake check
  SST_CHECK_STAKE([],[],[AC_MSG_ERROR([Stake requests but could not be found])])

  AS_IF([test "$miranda_happy" = "yes"], [$1], [$2])
])
