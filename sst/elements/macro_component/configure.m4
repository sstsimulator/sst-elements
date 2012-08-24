dnl -*- Autoconf -*-

AC_DEFUN([SST_MACRO_CONFIG], [
  AC_ARG_WITH([sstmacro],
    [AS_HELP_STRING([--with-sstmacro@<:@=DIR@:>@],
      [Use SST Macro package installed in optionally specified DIR])])

  happy="yes"

  AS_IF([test "$with_sstmacro" = "no"], [happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"

  AS_IF([test ! -z "$with_sstmacro" -a "$with_sstmacro" != "yes"],
    [SSTMACRO_CPPFLAGS="-I$with_sstmacro/include"
     CPPFLAGS="$SSTMACRO_CPPFLAGS $CPPFLAGS"
     SSTMACRO_LDFLAGS="-L$with_sstmacro/lib"
     LDFLAGS="$SSTMACRO_LDFLAGS $LDFLAGS"],
    [SSTMACRO_CPPFLAGS=
     SSTMACRO_LDFLAGS=
     happy="no"])

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([sstmac/sstmacro.h],[],[happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"

  AC_SUBST([SSTMACRO_CPPFLAGS])
  AC_SUBST([SSTMACRO_LDFLAGS])

  AS_IF([test "$happy" = "yes"], [$1], [$2])
])
