#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.60])
AC_INIT([miniMD], [0.1], [pscrozi@sandia.gov])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([config])
AC_CONFIG_SRCDIR([atom.cpp])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([foreign dist-bzip2 no-define 1.7])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_ARG_ENABLE([mpi],
			  [AS_HELP_STRING([--disable-mpi],
							  [disable the use of MPI])])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_LANG([C++])

AS_IF([test "x$enable_mpi" != xno],
	  [AX_MPI
	   AS_IF([test "$MPICXX"],
			 [CXX=$MPICXX
			  CXXLD=$MPICXX])])

# Checks for libraries.
#AC_CHECK_LIB([pthread], [pthread_create])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([pow sqrt])
MTGL_CHECK_QTHREADS

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
