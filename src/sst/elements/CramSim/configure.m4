dnl -*- Autoconf -*-
dnl vim:ft=config
dnl

AC_DEFUN([SST_CramSim_CONFIG], [
	cs_happy="yes"

  # Check for BOOST
  SST_CHECK_BOOST([],[cs_happy="no"])

  AS_IF([test "$cs_happy" = "yes"], [$1], [$2])
])
