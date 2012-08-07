dnl -*- Autoconf -*-

AC_DEFUN([SST_PhoenixSim_CONFIG], [
  AC_ARG_WITH([omnetpp],
    [AS_HELP_STRING([--with-omnetpp@<:@=DIR@:>@],
      [Use Omnet++ package installed in optionally specified DIR])])

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
  AC_CHECK_HEADERS([omnetpp.h],[OMNETPP_LIB="-loppsim $PHOENIXSIM_LDFLAGS "],[happy="no"])

  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"

  AC_SUBST([PHOENIXSIM_CPPFLAGS])
  AC_SUBST([PHOENIXSIM_LDFLAGS])
  AC_SUBST([OMNETPP_LIB])

  AS_IF([test "$happy" = "yes"], [$1], [$2])
])
