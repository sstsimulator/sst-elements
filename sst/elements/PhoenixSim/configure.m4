dnl -*- Autoconf -*-

AC_DEFUN([SST_PhoenixSim_CONFIG], [
  AC_ARG_WITH([omnetpp],
    [AS_HELP_STRING([--with-omnetpp@<:@=DIR@:>@],
      [Use Omnet++ package installed in optionally specified DIR])])

  AC_ARG_ENABLE([phoenixsim], [AS_HELP_STRING([--enable-phoenixsim], [Enable the PhoenixSim photonics simulation component])], 
	[], [enable_phoenixsim="no"])
  happy="yes"
  
  AS_IF([test "$with_omnetpp" = "no"], [happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"

  AS_IF([test ! -z "$with_omnetpp" -a "$with_omnetpp" != "yes"],
    [PHOENIXSIM_CPPFLAGS="-I$with_omnetpp/include"
     CPPFLAGS="$PHOENIXSIM_CPPFLAGS $CPPFLAGS"
     PHOENIXSIM_LDFLAGS="-L$with_omnetpp/lib -L$with_omnetpp/lib/gcc"     
     LDFLAGS="$PHOENIXSIM_LDFLAGS $LDFLAGS"],
    [PHOENIXSIM_CPPFLAGS=
     PHOENIXSIM_LDFLAGS=
     happy="no"])

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([omnetpp.h],[OMNETPP_LIB="-loppsim -loppcommon -loppnedxml $PHOENIXSIM_LDFLAGS "],[happy="no"])

  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  BUILD_PHOENIXSIM=""

  AS_IF( [test "$enable_phoenixsim" = "yes"],
	[AS_IF([test "$happy" = "no"], AC_MSG_FAILURE(["PhoenixSim enabled by user but system cannot locate OMNET libraries."]),
		 [AC_MSG_NOTICE(["PhoenixSim has been enabled by user"])])],
        [AC_MSG_NOTICE(["PhoenixSim has not been enabled by user, build is disabled"])] )

  AC_SUBST([PHOENIXSIM_CPPFLAGS])
  AC_SUBST([PHOENIXSIM_LDFLAGS])
  AC_SUBST([OMNETPP_LIB])
  AM_CONDITIONAL([BUILD_PHOENIXSIM], [test "$enable_phoenixsim" = "yes"])

  AS_IF([test "$happy" = "yes"], [$1], [$2])
])
