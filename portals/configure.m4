dnl -*- Autoconf -*-

AC_DEFUN([SST_portals_CONFIG], [
  AC_CHECK_FUNCS([setcontext getcontext])
  $1
])
