
AC_DEFUN([SST_CHECK_FDSIM], [
  AC_ARG_WITH([fdsim],
    [AS_HELP_STRING([--with-fdsim@<:@=DIR@:>@],
      [Use FlashDIMMSim package installed in optionally specified DIR])])

  sst_check_fdsim_happy="yes"
  AS_IF([test "$with_fdsim" = "no"], [sst_check_fdsim_happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"
  CXXFLAGS_saved="$CXXFLAGS"

  AS_IF([test ! -z "$with_fdsim" -a "$with_fdsim" != "yes"],
    [FDSIM_CPPFLAGS="-I$with_fdsim -I$with_fdsim/include"
     CPPFLAGS="$FDSIM_CPPFLAGS $CPPFLAGS"
     CXXFLAGS="$CXXFLAGS $SST_CXX0X_FLAGS"
     FDSIM_CXXFLAGS="$SST_CXX0X_FLAGS"
     FDSIM_LDFLAGS="-L$with_fdsim -L$with_fdsim/lib"
     FDSIM_LIBDIR="$with_fdsim"
     LDFLAGS="$FDSIM_LDFLAGS $LDFLAGS"],
    [FDSIM_CPPFLAGS=
     FDSIM_LDFLAGS=
     FDSIM_LIBDIR=
     FDSIM_CXXFLAGS=])

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([FlashDIMM.h], [], [sst_check_fdsim_happy="no"])
  AC_CHECK_LIB([fdsim], [main],
    [FDSIM_LIB="-lfdsim"], [sst_check_fdsim_happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"
  CXXFLAGS="$CXXFLAGS_saved"

  AC_SUBST([FDSIM_CPPFLAGS])
  AC_SUBST([FDSIM_LDFLAGS])
  AC_SUBST([FDSIM_CXXFLAGS])
  AC_SUBST([FDSIM_LIB])
  AC_SUBST([FDSIM_LIBDIR])
  AM_CONDITIONAL([HAVE_FDSIM], [test "$sst_check_fdsim_happy" = "yes"])
  AS_IF([test "$sst_check_fdsim_happy" = "yes"],
        [AC_DEFINE([HAVE_FDSIM], [1], [Set to 1 if FlashDIMMSim was found])])
  AC_DEFINE_UNQUOTED([FDSIM_LIBDIR], ["$FDSIM_LIBDIR"], [Path to FlashDIMMSim library])

  AS_IF([test "$sst_check_fdsim_happy" = "yes"], [$1], [$2])
])
