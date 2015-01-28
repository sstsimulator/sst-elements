AC_DEFUN([SST_SysC_CONFIG], [
  sst_sysc_comp_happy="yes"

  #check for SystemC runtime
  SST_CHECK_SYSTEMC([sst_sysc_comp_happy="yes"],[sst_sysc_comp_happy="no"],[AC_MSG_ERROR([SystemC requested but could not be found])])

  AS_IF([test "x$sst_sysc_comp_happy" = "xyes"], [$1], [$2])
])
