dnl -*- Autoconf -*-

AC_DEFUN([SST_ariel_CONFIG], [
  sst_check_ariel="yes"

  SST_CHECK_PINTOOL([have_pin=1],[have_pin=0],[AC_MSG_ERROR([PIN was requested but not found])])

  SST_CHECK_PTRACE_SET_TRACER()
  SST_CHECK_SHM()

  # Use Cuda
  SST_CHECK_CUDA()

  # Use LIBZ
  SST_CHECK_LIBZ()

  AC_SUBST([ARIEL_MPICC])
  AC_SUBST([ARIEL_MPICXX])
  AC_SUBST([ARIEL_MPI_CFLAGS])
  AC_SUBST([ARIEL_MPI_LIBS])

  AC_CONFIG_FILES([src/sst/elements/ariel/api/Makefile])
  AC_CONFIG_FILES([src/sst/elements/ariel/mpi/Makefile])
  AC_CONFIG_FILES([src/sst/elements/ariel/tests/testMPI/Makefile])
  AC_CONFIG_FILES([src/sst/elements/ariel/frontend/simple/examples/stream/Makefile])

  AS_IF([test "$sst_check_ariel" = "yes"], [$1], [$2])
])
