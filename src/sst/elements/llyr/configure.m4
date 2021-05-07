dnl -*- Autoconf -*-

AC_DEFUN([SST_llyr_CONFIG], [
  sst_check_llyrx="yes"

  SST_CHECK_LLVM_CONFIG([have_llvm_config=1],
                        [have_llvm_config=0],
                        [AC_MSG_ERROR([LLVM libraries required, but not found])])

  AS_IF([test "$have_llvm_config" = 1], [sst_check_llyrx="yes"], [sst_check_llyrx="no"])
  AS_IF([test "$sst_check_llyrx" = "yes"], [$1], [$2])
])
