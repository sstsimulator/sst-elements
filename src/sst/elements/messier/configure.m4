dnl -*- Autoconf -*-

AC_DEFUN([SST_messier_CONFIG], [
  sst_messier_comp_happy="yes"
AS_IF([test "x$sst_messier_comp_happy" = "xyes"], [$1], [$2])
])
