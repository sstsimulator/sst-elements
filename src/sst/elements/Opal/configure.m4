dnl -*- Autoconf -*-

AC_DEFUN([SST_Opal_CONFIG], [
  sst_Opal_comp_happy="yes"
AS_IF([test "x$sst_Opal_comp_happy" = "xyes"], [$1], [$2])
])
