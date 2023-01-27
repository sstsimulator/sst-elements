dnl -*- Autoconf -*-
dnl vim:ft=config
dnl

AC_DEFUN([SST_PandosProgramming_CONFIG], [
	pandosprogramming_happy="yes"

  # check pando runtime
  SST_CHECK_PANDO_RUNTIME([],[],[AC_MSG_ERROR(["Pando programming requested but could not be found])])

  AS_IF([test "$pandosprogramming_happy" = "yes"], [$1], [$2])
])
