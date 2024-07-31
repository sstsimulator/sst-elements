dnl -*- Autoconf -*-

AC_DEFUN([SST_ariel_CONFIG], [
  sst_check_ariel="yes"

  SST_CHECK_PINTOOL([have_pin=1],[have_pin=0],[AC_MSG_ERROR([PIN was requested but not found])])
dnl SST_CHECK_ARIEL_MPI([], [AC_MSG_ERROR([Can't enable Ariel MPI])])

  SST_CHECK_PTRACE_SET_TRACER()
  SST_CHECK_SHM()

  # Use Cuda
  SST_CHECK_CUDA()

  # Use LIBZ
  SST_CHECK_LIBZ()

  AC_SUBST([ARIEL_MPICC])
  AC_SUBST([ARIEL_MPICXX])
  AC_SUBST([ARIEL_CFLAGS])
  AC_SUBST([ARIEL_CXXFLAGS])

  AC_CONFIG_FILES([src/sst/elements/ariel/api/Makefile])
  AC_CONFIG_FILES([src/sst/elements/ariel/mpi/Makefile])

  AS_IF([test "$sst_check_ariel" = "yes"], [$1], [$2])
])
