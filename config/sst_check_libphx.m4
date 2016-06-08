

AC_DEFUN([SST_CHECK_LIBPHX], [
  AC_ARG_WITH([libphx],
    [AS_HELP_STRING([--with-libphx@<:@=DIR@:>@],
      [Use libphx package installed in optionally specified DIR])])

  sst_check_libphx_happy="yes"
  AS_IF([test "$with_libphx" = "no"], [sst_check_libphx_happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"

  AS_IF([test ! -z "$with_libphx" -a "$with_libphx" != "yes"],
    [LIBPHX_CPPFLAGS="-I$with_libphx"
     CPPFLAGS="$LIBPHX_CPPFLAGS $CPPFLAGS"
     LIBPHX_LDFLAGS="-L$with_libphx"
     LIBPHX_LIBDIR="$with_libphx"
     LDFLAGS="$LIBPHX_LDFLAGS $LDFLAGS"],
    [LIBPHX_CPPFLAGS=
     LIBPHX_LDFLAGS=
     LIBPHX_LIBDIR=])

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([Vault.h], [], [sst_check_libphx_happy="no"])
  AC_CHECK_LIB([phx], [vault_is_present],
    [LIBPHX_LIB="-lphx"], [sst_check_libphx_happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"

  AC_SUBST([LIBPHX_CPPFLAGS])
  AC_SUBST([LIBPHX_LDFLAGS])
  AC_SUBST([LIBPHX_LIB])
  AC_SUBST([LIBPHX_LIBDIR])
  AM_CONDITIONAL([HAVE_LIBPHX], [test "$sst_check_libphx_happy" = "yes"])
  AS_IF([test "$sst_check_libphx_happy" = "yes"],
        [AC_DEFINE([HAVE_LIBPHX], [1], [Set to 1 if LIBPHX was found])])
  AC_DEFINE_UNQUOTED([LIBPHX_LIBDIR], ["$LIBPHX_LIBDIR"], [Path to LIBPHX library])

  AS_IF([test "$sst_check_libphx_happy" = "yes"], [$1], [$2])
])
