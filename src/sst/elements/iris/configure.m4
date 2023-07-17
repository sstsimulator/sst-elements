dnl -*- Autoconf -*-

AC_DEFUN([SST_iris_CONFIG],
[
  sst_check_iris="yes"
  AS_IF([test "$sst_check_iris" = "yes"], [$1], [$2])
  AC_CONFIG_FILES([src/sst/elements/iris/libfabric/Makefile])
])
