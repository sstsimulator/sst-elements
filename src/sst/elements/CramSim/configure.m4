dnl -*- Autoconf -*-
dnl vim:ft=config
dnl

AC_DEFUN([SST_CramSim_CONFIG], [
	cs_happy="yes"

  AS_IF([test "$cs_happy" = "yes"], [$1], [$2])
])
