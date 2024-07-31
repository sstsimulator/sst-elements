AC_DEFUN([SST_CHECK_ARIEL_MPI], [
  sst_check_ariel_mpi_happy="no"

  AC_ARG_ENABLE([ariel-mpi],
    [AS_HELP_STRING([--enable-ariel-mpi],
    [Enable MPI support in Ariel [default=no]])])

  AS_IF([test "$enable_ariel_mpi" = "yes"], [sst_check_ariel_mpi_happy="yes"])

  dnl Find the MPI compilers and put them in MPICC and MIPCXX
  AS_IF([test "$sst_check_ariel_mpi_happy" = "yes"], [
    AC_LANG_PUSH([C])
    ACX_MPI([], [sst_check_ariel_mpi_happy="no"])
    AC_LANG_POP([C])
    ])

  AS_IF([test "$sst_check_ariel_mpi_happy" = "yes"], [
    AC_LANG_PUSH([C++])
    ACX_MPI([], [sst_check_ariel_mpi_happy="no"])
    AC_LANG_POP([C++])
    ])

  dnl Ensure Core was compiled without MPI
  dnl todo
  dnl

  AX_OPENMP()
  ARIEL_CFLAGS = $OPENMP_CFLAGS
  ARIEL_CXXFLAGS = $OPENMP_CXXFLAGS
  $OPENMP_CFLAGS

  ARIEL_MPICC=$MPICC
  ARIEL_MPICXX=$MPICXX
  AM_CONDITIONAL([SST_USE_ARIEL_MPI], [test "$sst_check_ariel_mpi_happy" = "yes"])
])

