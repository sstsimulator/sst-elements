dnl -*- Autoconf -*-

AC_DEFUN([SST_sstdisksim_CONFIG], [
  AC_ARG_WITH([disksim],
    [AS_HELP_STRING([--with-disksim@<:@=DIR@:>@],
      [Use DiskSim package installed in optionally specified DIR])])

  happy="yes"

  AS_IF([test "$with_disksim" = "no"], [happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"

  AS_IF([test ! -z "$with_disksim" -a "$with_disksim" != "yes"],
    [SSTDISKSIM_CPPFLAGS="-I$with_disksim"
     CPPFLAGS="$SSTDISKSIM_CPPFLAGS $CPPFLAGS"
     SSTDISKSIM_LDFLAGS="-L$with_disksim"     
     LDFLAGS="$SSTDISKSIM_LDFLAGS $LDFLAGS"],
    [SSTDISKSIM_CPPFLAGS=
     SSTDISKSIM_LDFLAGS=])

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([disksim_interface.h], [SSTDISKSIM_LIB="-ldisksim"], [happy="no"])
#  SSTDISKSIM_LIB="-ldisksim"
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"

  AC_SUBST([SSTDISKSIM_CPPFLAGS])
  AC_SUBST([SSTDISKSIM_LDFLAGS])
  AC_SUBST([SSTDISKSIM_LIB])

  AS_IF([test "$happy" = "yes"], [$1], [$2])
])
