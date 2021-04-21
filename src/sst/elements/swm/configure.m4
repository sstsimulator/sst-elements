dnl -*- Autoconf -*-
dnl vim:ft=config
dnl

AC_DEFUN([SST_swm_CONFIG], [
	swm_happy="yes"

  SST_CHECK_SWM([],[],[AC_MSG_ERROR([SWM could not be found])])

  AS_IF([test "$swm_happy" = "yes"], [$1], [$2])
])
