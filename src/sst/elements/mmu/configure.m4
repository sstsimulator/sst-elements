dnl -*- Autoconf -*-

AC_DEFUN([SST_mmu_CONFIG], [
  sst_mmu_comp_happy="yes"
AS_IF([test "x$sst_mmu_comp_happy" = "xyes"], [$1], [$2])
])
