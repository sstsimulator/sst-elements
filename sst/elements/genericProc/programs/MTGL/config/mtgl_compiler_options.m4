dnl MTGL compiler options
dnl
dnl   MTGL_COMPILER_OPTIONS()
dnl
dnl   Use "subpackage" option to set flags to defaults and ignore 
dnl   command line arguments.  It leaves command line options unset
dnl   and uses the compiler flags in the environment, which were
dnl   computed by the parent package.
dnl

AC_DEFUN([MTGL_COMPILER_OPTIONS],[
  dnl
  dnl An explicit -with-*flags= overrides our default flags and any
  dnl flags that be defined in the environment.
  dnl --without-debugging omits debugging flag from flags
  dnl --with-optimization=flag replaces optimization flag with "flag"
  dnl --without-optimization or --with-optimization=no changes to -O0
  dnl 
  
  AC_ARG_WITH([cppflags],
    [AS_HELP_STRING([--with-cppflags],
       [set preprocessor flags (e.g., "-Dflag -Idir")])],
    [CPPFLAGS="${withval}"
     AC_MSG_NOTICE([--with-cppflags is deprecated - flags should be added directly to CPPFLAGS])])
  
  AC_ARG_WITH([cflags],
    [AS_HELP_STRING([--with-cflags], [set C compiler flags])],
    [CFLAGS="${withval}"
     AC_MSG_NOTICE([--with-cflags is deprecated - flags should be added directly to CFLAGS])])
  
  AC_ARG_WITH([cxxflags],
    [AS_HELP_STRING([--with-cxxflags], [set C++ compiler flags])],
    [CXXFLAGS="${withval}"
     AC_MSG_NOTICE([--with-cxxflags is deprecated - flags should be added directly to CXXFLAGS])])
  
  AC_ARG_WITH([ldflags],
    [AS_HELP_STRING([--with-ldflags],[add extra linker (typically -L) flags])],
    [LDFLAGS="${withval}"
     AC_MSG_NOTICE([--with-ldflags is deprecated - flags should be added directly to LDFLAGS])])
  
  AC_ARG_WITH([libs],
    [AS_HELP_STRING([--with-libs], [add extra library (typically -l) flags])],
    [LIBS="${withval}"
     AC_MSG_NOTICE([--with-libs is deprecated - flags should be added directly to LIBS])])
  
  AC_ARG_WITH([debugging],
    [AS_HELP_STRING([--with-debugging],
      [Add extra debugging flags to CFLAGS/CXXFLAGS])],
    [with_debugging=$withval], [with_debugging=no])
  AS_IF([test "$with_debugging" != "no"],
    [AS_IF([test "$with_debugging" = "yes"],
      [DEBUGGING_FLAGS=-g],
      [DEBUGGING_FLAGS="$with_debugging"])
     CFLAGS="$DEBUGGING_FLAGS $CFLAGS"
     CXXFLAGS="$DEBUGGING_FLAGS $CXXFLAGS"
     AC_MSG_NOTICE([--with-debugging is deprecated - flags should be added directly to CFLAGS/CXXFLAGS])])
  
  AC_ARG_WITH([optimization],
    [AS_HELP_STRING([--with-optimization=flag],
      [Build with the specified optimization flag.])],
    [with_optimization=$withval], [with_optimization=no])
  AS_IF([test "$with_optimization" != "no"],
    [AS_IF([test "$with_optimization" = "yes"],
      [OPTIMIZATION_FLAGS=-O2],
      [OPTIMIZATION_FLAGS="$with_optimization"])
     CFLAGS="$OPTIMIZATION_FLAGS $CFLAGS"
     CXXFLAGS="$OPTIMIZATION_FLAGS $CXXFLAGS"
     AC_MSG_NOTICE([--with-optimization is deprecated - flags should be added directly to CFLAGS/CXXFLAGS])])

])
