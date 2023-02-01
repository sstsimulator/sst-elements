AC_DEFUN([SST_CHECK_PANDO_RUNTIME], [
  # create argument --with-pando-runtime= to track insstall location
  AC_ARG_WITH([pando_runtime],
    [AS_HELP_STRING([--with-pando-runtime@<:@=DIR@:>@],
      [Use pando-runtime package installed in optionally specified DIR])])

  # check if pando-runtime has been set
  sst_check_pando_runtime_happy="yes"
  AS_IF([test "$with_pando_runtime" = "no"], [sst_check_pando_runtime_happy="no"])

  # set flags
  CXXFLAGS_saved="$CXXFLAGS"
  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"
  AS_IF([test ! -z "$with_pando_runtime" -a "$with_pando_runtime" != "yes"],
    # set cppflags
    [PANDO_RUNTIME_CPPFLAGS="-I$with_pando_runtime/include"
     CPPFLAGS="$PANDO_RUNTIME_CPPFLAGS $AM_CPPFLAGS $CPPFLAGS"
     # set cxxflags
     PANDO_RUNTIME_CXXFLAGS="-std=c++14"
     CXXFLAGS="$PANDO_RUNTIME_CXXFLAGS $CXXFLAGS"
     # set ldflags
     PANDO_RUNTIME_LDFLAGS="-L$with_pando_runtime/lib -Wl,-rpath=$with_pando_runtime/lib"
     PANDO_RUNTIME_LIBDIR="$with_pando_runtime/lib"
     LDFLAGS="$PANDO_RUNTIME_LDFLAGS $AM_LDFLAGS $LDFLAGS"],
    [PANDO_RUNTIME_CPPFLAGS=
     PANDO_RUNTIME_LDFLAGS=
     PANDO_RUNTIME_LIBDIR=])

  # configure
  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([pando/pando.hpp], [], [sst_check_pando_runtime_happy="no"])
  AC_CHECK_LIB([pando-rtl-backend], [libpandoruntime_is_present],
    [PANDO_RUNTIME_LIB="-lpando-rtl -lpando-rtl-backend -lpando-rtl-arch"], [sst_check_pando_runtime_happy="no"])
  AC_LANG_POP(C++)

  # restore saved flags
  CXXFLAGS="$CXXFLAGS_saved"
  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"

  AC_SUBST([PANDO_RUNTIME_CPPFLAGS])
  AC_SUBST([PANDO_RUNTIME_LDFLAGS])
  AC_SUBST([PANDO_RUNTIME_LIB])
  AC_SUBST([PANDO_RUNTIME_LIBDIR])
  AM_CONDITIONAL([HAVE_PANDO_RUNTIME], [test "$sst_check_pando_runtime_happy" = "yes"])
  AS_IF([test "$sst_check_pando_runtime_happy" = "yes"],
        [AC_DEFINE([HAVE_PANDO_RUNTIME], [1], [Set to 1 if pando-runtime was found])])
  AC_DEFINE_UNQUOTED([PANDO_RUNTIME_LIBDIR], ["$PANDO_RUNTIME_LIBDIR"], [Path to pando-runtime library])

  AS_IF([test "$sst_check_pando_runtime_happy" = "yes"], [$1], [$2])
  
])
