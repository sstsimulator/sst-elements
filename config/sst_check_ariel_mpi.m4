AC_DEFUN([SST_CHECK_ARIEL_MPI], [
  sst_check_ariel_mpi_happy="no"

  AC_ARG_ENABLE([ariel-mpi],
    [AS_HELP_STRING([--enable-ariel-mpi],
    [Enable MPI support in Ariel [default=no]])])

  AS_IF([test "$enable_ariel_mpi" = "yes"], [sst_check_ariel_mpi_happy="yes"])

  dnl Ensure Core was compiled without MPI
  dnl Regrettably, this runs before we have checked whether sst-config exists,
  dnl as that config file overwrites the MPICXX and MPICC variables needed by
  dnl the ACX_MPI macro. We plan to remove this check altogether in the future.
  AS_IF([test "$sst_check_ariel_mpi_happy" = "yes"], [
    AC_MSG_CHECKING([whether sst-core was compilied without MPI])
    sst_config_out=$(sst-config --MPI_CPPFLAGS)
    if test -z "$sst_config_out"; then
      AC_MSG_RESULT([yes])
    else
      AC_MSG_RESULT([no])
      AC_MSG_WARN([SST-Core appears to have been compiled with MPI support. Disabling Ariel MPI support.])
      sst_check_ariel_mpi_happy="no"
    fi
    ])

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

  dnl Elements will overwrite these with the values used for compiling Core. We
  dnl will save them in new variables.
  ARIEL_MPICC=$MPICC
  ARIEL_MPICXX=$MPICXX
  AS_IF([test "$sst_check_ariel_mpi_happy" = "yes"], [
    ARIEL_MPI_CFLAGS=$($MPICC -showme:compile)
    ARIEL_MPI_LIBS=$($MPICC -showme:link)
    ])

  AS_IF([test "$sst_check_ariel_mpi_happy" = "yes"], [
    AC_DEFINE([ENABLE_ARIEL_MPI], [1], [Enable Ariel MPI features])
    ])
  AM_CONDITIONAL([SST_USE_ARIEL_MPI], [test "$sst_check_ariel_mpi_happy" = "yes"])
])

