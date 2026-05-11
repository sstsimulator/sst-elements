dnl -*- Autoconf -*-
dnl vim:ft=config
dnl

AC_DEFUN([SST_carcosa_CONFIG], [
	carcosa_happy="yes"

  AS_IF([test "$carcosa_happy" = "yes"], [$1], [$2])
])
