
AC_DEFUN([SST_CHECK_RAMULATOR2], [
  AC_ARG_WITH([ramulator2],
    [AS_HELP_STRING([--with-ramulator2@<:@=DIR@:>@],
      [Use Ramulator2 library installed in optionally specified DIR])])

  sst_check_ramulator2_happy="yes"
  AS_IF([test "$with_ramulator2" = "no"], [sst_check_ramulator2_happy="no"])

  CXXFLAGS_saved="$CXXFLAGS"
  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"

  AS_IF([test ! -z "$with_ramulator2" -a "$with_ramulator2" != "yes"],
    [RAMULATOR2_CPPFLAGS="-I$with_ramulator2/src/ -I$with_ramulator2/ext/spdlog/include/ \
      -I$with_ramulator2/ext/yaml-cpp/include/ -DRAMULATOR2 -DHAVE_RAMULATOR2"
     CPPFLAGS="$RAMULATOR2_CPPFLAGS $AM_CPPFLAGS $CPPFLAGS"
     CXXFLAGS="$AM_CXXFLAGS $CXXFLAGS"
     RAMULATOR2_LDFLAGS="-L$with_ramulator2"
     RAMULATOR2_LIBDIR="$with_ramulator2"
     LDFLAGS="$RAMULATOR2_LDFLAGS $AM_LDFLAGS $LDFLAGS"],
    [RAMULATOR2_CPPFLAGS=
     RAMULATOR2_LDFLAGS=
     RAMULATOR2_LIBDIR=])

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([base/base.h], [], [sst_check_ramulator2_happy="no"])
  AC_CHECK_FILE([$with_ramulator2/libramulator.so], #[_ZZN4YAML3Exp3TagEvE1e],
    [sst_check_ramulator2_happy="yes"], [sst_check_ramulator2_happy="no"])
  AS_IF([test "$sst_check_ramulator2_happy" = "yes"],
        [RAMULATOR2_LIB="-lramulator"])
  AC_LANG_POP(C++)

  CXXFLAGS="$CXXFLAGS_saved"
  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"

  AC_SUBST([RAMULATOR2_CPPFLAGS])
  AC_SUBST([RAMULATOR2_LDFLAGS])
  AC_SUBST([RAMULATOR2_LIB])
  AC_SUBST([RAMULATOR2_LIBDIR])
  AM_CONDITIONAL([HAVE_RAMULATOR2], [test "$sst_check_ramulator2_happy" = "yes"])
  AS_IF([test "$sst_check_ramulator2_happy" = "yes"],
        [AC_DEFINE([HAVE_RAMULATOR2], [1], [Set to 1 if Ramulator2 was found])])
  AC_DEFINE_UNQUOTED([RAMULATOR2_LIBDIR], ["$RAMULATOR2_LIBDIR"], [Path to Ramulator2 library])

  AS_IF([test "$sst_check_ramulator2_happy" = "yes"], [$1], [$2])
])
