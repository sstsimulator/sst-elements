dnl -*- Autoconf -*-

AC_DEFUN([SST_samba_CONFIG], [
  sst_samba_comp_happy="yes"
AS_IF([test "x$sst_samba_comp_happy" = "xyes"], [$1], [$2])
])
