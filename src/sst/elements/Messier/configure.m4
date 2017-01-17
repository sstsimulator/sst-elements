dnl -*- Autoconf -*-

AC_DEFUN([SST_Messier_CONFIG], [
  sst_Messier_comp_happy="yes"
AS_IF([test "x$sst_Messier_comp_happy" = "xyes"], [$1], [$2])
])
