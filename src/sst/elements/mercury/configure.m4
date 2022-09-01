dnl -*- Autoconf -*-

AC_DEFUN([SST_mercury_CONFIG],
[
  sst_check_mercury="yes"
  AM_CONDITIONAL(SST_HG_X86, [test "X$host_cpu" = "Xx86_64"])
  AM_CONDITIONAL(SST_HG_A64, [test "X$host_cpu" = "Xarm64" -o "X$host_cpu" = "Xaarch64"])
  AS_IF([test "$sst_check_mercury" = "yes"], [$1], [$2])
])
