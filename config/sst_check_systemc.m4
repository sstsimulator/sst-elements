AC_DEFUN([SST_CHECK_SYSTEMC], [
  AC_ARG_WITH([systemc],
    [AS_HELP_STRING([--with-systemc@<:@=DIR@:>@],
      [Use SystemC runtime installed in optionally specified DIR])])

  sst_check_systemc_happy="yes"
  AS_IF([test "$with_systemc" = "no"], [sst_check_systemc_happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"

  AS_IF([test ! -z "$with_systemc" -a "$with_systemc" != "yes"],
    [SST_SYSTEMC_CPPFLAGS="-I$with_systemc/include"
     CPPFLAGS="$SST_SYSTEMC_CPPFLAGS $CPPFLAGS"
     SST_SYSTEMC_LDFLAGS="-L$with_systemc/lib -L$with_systemc/lib-linux64"
     SST_SYSTEMC_LIBDIR="$with_systemc/lib"
     LDFLAGS="$SST_SYSTEMC_LDFLAGS $LDFLAGS"],
    [SST_SYSTEMC_CPPFLAGS=
     SST_SYSTEMC_LDFLAGS=
     SST_SYSTEMC_LIBDIR=])

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([systemc.h], [], [sst_check_systemc_happy="no"])
# AC_CHECK_LIB([systemc], [sc_argv],
#    [SST_SYSTEMC_LIB="-lsystemc"], [sst_check_systemc_happy="no"])
  LIBS="-lsystemc $LIBS"
  LDFLAGS="-lsystemc $LDFLAGS"
  AC_LINK_IFELSE([
                 AC_LANG_SOURCE(
                     [[#include <systemc.h>
                     int sc_main(int argc, char* argv[]){
                     return 0;
                     }
                     int main(int argc,char* argv[]){
                     return sc_main(argc,argv);
                     }
                     ]]
                     )],[SST_SYSTEMC_LIB="-lsystemc"], [sst_check_systemc_happy="no"])
#TODO test to see if patch was applied and throw a specific error
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"

  AC_SUBST([SST_SYSTEMC_CPPFLAGS])
  AC_SUBST([SST_SYSTEMC_LDFLAGS])
  AC_SUBST([SST_SYSTEMC_LIB])
  AC_SUBST([SST_SYSTEMC_LIBDIR])
  AM_CONDITIONAL([HAVE_SST_SYSTEMC], [test "$sst_check_systemc_happy" = "yes"])
  AS_IF([test "$sst_check_systemc_happy" = "yes"],
        [AC_DEFINE([HAVE_SST_SYSTEMC], [1], [Set to 1 if SystemC was found])])
  AC_DEFINE_UNQUOTED([SST_SYSTEMC_LIBDIR], ["$SST_SYSTEMC_LIBDIR"], [Path to SystemC library])

  AS_IF([test "$sst_check_systemc_happy" = "yes"], [$1], [$2])
])
