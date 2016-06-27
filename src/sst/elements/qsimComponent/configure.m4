dnl -*- Autoconf -*-

AC_DEFUN([SST_qsimComponent_CONFIG], [
  sst_qsim_comp_happy="yes"

  #check for  runtime
  SST_CHECK_QSIM([sst_qsim_comp_happy="yes"],[sst_qsim_comp_happy="no"],[AC_MSG_ERROR([qsim requested but could not be found])])

  AS_IF([test "x$sst_qsim_comp_happy" = "xyes"], [$1], [$2])
])
