dnl -*- Autoconf -*-

AC_DEFUN([SST_iris_CONFIG],
[
  sst_check_iris="yes"
  AC_CONFIG_FILES([src/sst/elements/iris/libfabric/Makefile])
  AS_IF([test "$sst_check_iris" = "yes"], [$1], [$2])
])
