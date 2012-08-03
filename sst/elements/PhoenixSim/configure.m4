dnl -*- Autoconf -*-

AC_DEFUN([SST_PhoenixSim_CONFIG], [
  AC_ARG_WITH([phoenixsim],
    [AS_HELP_STRING([--with-phoenixsim@<:@=DIR@:>@],
      [Use PhoenixSim package installed in optionally specified DIR])])

  happy="yes"
  AS_IF([test "$with_phoenixsim" = "no"], [happy="no"])
  
  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"

  AS_IF([test ! -z "$with_phoenixsim" -a "$with_phoenixsim" != "yes"],
    [PHOENIXSIM_CPPFLAGS="$with_phoenixsim"
     CPPFLAGS="$PHOENIXSIM_CPPFLAGS $CPPFLAGS"
     PHOENIXSIM_LDFLAGS="$with_phoenixsim"
     LDFLAGS="$PHOENIXSIM_LDFLAGS $LDFLAGS"])

  AC_LANG_PUSH(C++)
  AC_TRY_LINK([#include <omnetpp.h>],[],[happy="no" echo "Error! Omnet++ lib not found. LD_LIBRARY_PATH set to ${LD_LIBRARY_PATH}"
        exit -1])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"

  AC_SUBST([PHOENIXSIM_CPPFLAGS])
  AC_SUBST([PHOENIXSIM_LDFLAGS])
  AC_SUBST([PHOENIXSIM_LIB])

  AS_IF([test "$happy" = "yes"], [$1], [$2])
])
