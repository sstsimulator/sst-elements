dnl -*- Autoconf -*-

AC_DEFUN([SST_chdlComponent_CONFIG], [
  sst_chdl_comp_happy="yes"

  #check for chdl runtime
  SST_CHECK_CHDL([sst_chdl_comp_happy="yes"],[sst_chdl_comp_happy="no"],[AC_MSG_ERROR([chdl requested but could not be found])])

  AS_IF([test "x$sst_chdl_comp_happy" = "xyes"], [$1], [$2])
])

