#
# Script to be processed by autoconf to produce a configure script
# and config.h header file.
#

###############################################################
# Init Autoconf / Automake
#

AC_PREREQ(2.59)
AC_INIT([mtgl], [0.9.1], [eldorado-graph@sandia.gov])
AC_CONFIG_MACRO_DIR([config])
AC_CONFIG_AUX_DIR([config])
AM_INIT_AUTOMAKE([1.9.6 foreign dist-bzip2 no-define tar-ustar])

CFLAGS=${CFLAGS-"-O3"}
CXXFLAGS=${CXXFLAGS-"-O3"}

dnl information on this package
AC_CONFIG_SRCDIR([mtgl/graph.hpp])
AC_CONFIG_HEADER([mtgl/mtgl_config.h])

dnl host information
AC_CANONICAL_BUILD
AC_CANONICAL_HOST

dnl Configure command line options

MTGL_COMPILER_OPTIONS
MTGL_COMPILER_OPTION

# --with-traceview flag (default=off)
AC_ARG_WITH(traceview,
    AC_HELP_STRING([--with-traceview],[compile with traceivew enabled]),
    [case "${withval}" in
	yes)
	    echo "--with-traceview = yes"
	    traceview=true
	    ;;
	no)
	    echo "--with-traceview = no"
	    traceview=false
	    ;;
	*)
	    AC_MSG_ERROR(bad value in ${withval} for --with-traceview)
	    ;;
    esac],
    [traceview=false]
)
AM_CONDITIONAL([WITH_TRACEVIEW],[ test "$traceview" = true ])

# --with_debug_arraybounds (default=off)
#   compiles with -DCHECK_BOUNDS to control bounds checking in 
#   dynamicarrays.
AC_ARG_WITH(debug_arraybounds,
    AC_HELP_STRING([--with-debug_arraybounds],[Enable array bounds check in DynamicArray]),
    [case "${withval}" in
	yes)
	    echo "--with-debug_arraybounds = yes"
	    debug_arraybounds=true
	    ;;
	no)
	    echo "--with-debug_arraybounds = no"
	    debug_arraybounds=false
	    ;;
	*)
	    AC_MSG_ERROR(bad value in ${withval} for --with-debug_arraybounds)
	    ;;
    esac],
    [debug_arraybounds=false]
)
AM_CONDITIONAL([WITH_DEBUG_ARRAYBOUNDS],[ test "$debug_arraybounds" = true ])

AC_ARG_ENABLE([releasable-only],
  [AC_HELP_STRING([--enable-releasable-only],
     [Only attempt to build releasable code.  Defaults to no if
  building from SVN, yes otherwise.  This option will result in an
  unbuildable tree if specified from a release tarball.])])
AS_IF([test -z "$enable_releasable_only"],
  [AS_IF([test -r "$srcdir/.svn"],
     [enable_releasable_only=no],
     [enable_releasable_only=yes])])
AM_CONDITIONAL([RELEASABLE_CODE_ONLY], [test $enable_releasable_only = yes])


###############################################################
# Checks for programs
#

dnl list cc/c++ first so that we default to building for the xmt
dnl compute nodes rather than the head nodes.
AC_PROG_CC([cc gcc])
AC_PROG_CC_C_O
AC_PROG_CXX([c++ CC g++])
AC_PROG_CPP

AC_PROG_LN_S
AM_PROG_AS
AC_PROG_RANLIB

AC_CACHE_CHECK([whether targeting MTA/XMT],
    [mtgl_cv_on_mta],
    [AC_TRY_COMPILE([], [ return __MTA__; ], [mtgl_cv_on_mta=yes], [mtgl_cv_on_mta=no])])
AM_CONDITIONAL(ON_MTA, [test "$mtgl_cv_on_mta" == "yes"])
AS_IF([test "${mtgl_cv_on_mta}" = yes],[
   CFLAGS="${CFLAGS} -par "
   CXXFLAGS="${CXXFLAGS} -par "
   LIBS="${LIBS} -lm -lprand"
],[
   AC_SEARCH_LIBS([sin], [m m_ppc])
])


###############################################################
# Checks for libraries
#

MTGL_CHECK_QTHREADS


###############################################################
# checks for headers
#

AC_HEADER_STDC

AC_CHECK_HEADERS([malloc.h])

###############################################################
# checks for types
#


###############################################################
# checks for structures
#


###############################################################
# checks for compiler characteristics
  

###############################################################
# checks for library functions

AC_CHECK_FUNCS([stricmp lrand48 rand_s strcasecmp realpath calloc])


###############################################################
# Output of autoconf/automake
#

dnl Last minute fix-up so that #include <mtgl/*.hpp> always works
CPPFLAGS='-I$(top_srcdir) -I$(top_builddir)'" $CPPFLAGS"

AC_CONFIG_FILES([Makefile mtgl/Makefile test/Makefile])

AC_MSG_NOTICE(CC=${CC})
AC_MSG_NOTICE(CXX=${CXX})
AC_MSG_NOTICE(CPPFLAGS=${CPPFLAGS})
AC_MSG_NOTICE(CFLAGS=${CFLAGS})
AC_MSG_NOTICE(CXXFLAGS=${CXXFLAGS})
AC_MSG_NOTICE(LDFLAGS=${LDFLAGS})
AC_MSG_NOTICE(LIBS=${LIBS})

AC_OUTPUT
