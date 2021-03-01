dnl -*- Autoconf -*-

AC_DEFUN([SST_balar_CONFIG], [
  sst_check_balarx="yes"

  SST_CHECK_GPGPUSIM([have_gpgpusim=1],[have_gpgpusim=0],[AC_MSG_ERROR([GPGPU-Sim required, but not found])])

  AS_IF([test "$have_gpgpusim" = 1], [sst_check_balarx="yes"], [sst_check_balarx="no"])
  AS_IF([test "$sst_check_balarx" = "yes"], [$1], [$2])
])
