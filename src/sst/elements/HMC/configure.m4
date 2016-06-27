dnl -*- Autoconf -*-

AC_DEFUN([SST_HMC_CONFIG], [
  sst_hmc_comp_happy="yes"

  #check for SystemC runtime
  SST_CHECK_MICRON_HMCSIM([sst_hmc_comp_happy="yes"],[sst_hmc_comp_happy="no"],[AC_MSG_ERROR([Micron HMC Sim requested but could not be found])])

  AS_IF([test "x$sst_hmc_comp_happy" = "xyes"], [$1], [$2])
])
