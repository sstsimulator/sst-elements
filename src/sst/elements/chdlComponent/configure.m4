dnl -*- Autoconf -*-

AC_DEFUN([SST_chdlComponent_CONFIG],[

  AC_ARG_WITH([chdl],
    [AS_HELP_STRING([--with-chdl@<:@=DIR@:>@],
      [Use CHDL installed at optionally-specified prefix DIR])])

  sst_check_chdl_happy="yes"

  AS_IF([test "$with_chdl" = "no"], [sst_check_chdl_happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  CXXFLAGS_saved="$CXXFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"

  AS_IF([test ! -z "$with_chdl" -a "$with_chdl" != "yes"],
    [CHDL_CPPFLAGS="-I$with_chdl/include -std=c++11"
     CPPFLAGS="$CHDL_CPPFLAGS $CPPFLAGS"
     CHDL_CXXFLAGS="-std=c++11"
     CXXFLAGS="$CHDL_CXXFLAGS $CXXFLAGS"
     CHDL_LDFLAGS="-L$with_chdl/lib"
     LDFLAGS="$CHDL_LDFLAGS $LDFLAGS"
     CHDL_LIBDIR="$with_chdl/lib"],
    [CHDL_CPPFLAGS=
     CHDL_CXXFLAGS=
     CHDL_LDFLAGS=])

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([chdl/chdl.h], [], [sst_check_chdl_happy="no"])
  AC_CHECK_LIB([chdl], [chdl_present],
    [CHDL_LIBS="-lchdl"], [sst_check_chdl_happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  CXXFLAGS="$CXXFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"

  AC_SUBST([CHDL_CPPFLAGS])
  AC_SUBST([CHDL_CXXFLGAS])
  AC_SUBST([CHDL_LDFLAGS])
  AC_SUBST([CHDL_LIBS])
  AC_SUBST([CHDL_LIBDIR])
  AM_CONDITIONAL([HAVE_CHDL], [test "$sst_check_chdl_happy" = "yes"])
  AS_IF([test "$sst_check_chdl_happy" = "yes"],
        [AC_DEFINE([HAVE_CHDL], [1], [Set to 1 if chdl was found])])
  AC_DEFINE_UNQUOTED([CHDL_LIBDIR], ["$CHDL_LIBDIR"], [Path to chdl library])



  AS_IF([test "$sst_check_chdl_happy" = "yes"], [$1], [$2])
])
