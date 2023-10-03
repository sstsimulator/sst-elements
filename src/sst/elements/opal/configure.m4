dnl -*- Autoconf -*-

AC_DEFUN([SST_opal_CONFIG], [
  sst_opal_comp_happy="yes"
AS_IF([test "x$sst_opal_comp_happy" = "xyes"], [$1], [$2])
])
