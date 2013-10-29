dnl -*- Autoconf -*-

AC_DEFUN([SST_introspector_cpu_CONFIG], [
  SST_CHECK_USING_MPI([$1],
      [AC_MSG_WARN([Introspector_cpu does not support non-MPI builds])
       $2])
])
