AC_DEFUN([SST_CHECK_STAKE],
[
  sst_check_stake_happy="yes"

  AC_ARG_WITH([stake],
    [AS_HELP_STRING([--with-stake@<:@=DIR@:>@],
      [Use RISC-V Spike simulator installed in optionally specified DIR])])

  AS_IF([test "$with_stake" = "no"], [sst_check_stake_happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"

  AS_IF([test "$sst_check_stake_happy" = "yes"], [
    AS_IF([test ! -z "$with_stake" -a "$with_stake" != "yes"],
      [STAKE_CPPFLAGS="-I$with_stake/include -I$with_stake/include/spike -I$with_stake/include/fesvr -I$with_stake/riscv64-unknown-elf/include/riscv-pk"
       CPPFLAGS="$STAKE_CPPFLAGS $CPPFLAGS"
       STAKE_LDFLAGS="-L$with_stake/lib -L$with_stake"
       STAKE_LIB="-lriscv -lpthread -lfesvr -ldl",
       LDFLAGS="$STAKE_LDFLAGS $LDFLAGS"
       STAKE_LIBDIR="$with_stake/lib"],
      [STAKE_CPPFLAGS=
       STAKE_LDFLAGS=
       STAKE_LIB=
       STAKE_LIBDIR=])])

  AC_LANG_PUSH([C++])
  AC_CHECK_HEADER([sim.h], [], [sst_check_stake_happy="no"])
  AC_LANG_POP([C++])

  AC_CHECK_LIB([riscv], [_ZN5sim_t3runEv],
      [STAKE_LIB="-lriscv -lfesvr -lsoftfloat"], [sst_check_stake_happy="no"], [-lfesvr -lsoftfloat])

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"

  AC_SUBST([STAKE_CPPFLAGS])
  AC_SUBST([STAKE_LDFLAGS])
  AC_SUBST([STAKE_LIB])
  AC_SUBST([STAKE_LIBDIR])
  AS_IF([test "x$sst_check_stake_happy" = "xyes"], [AC_DEFINE([HAVE_STAKE],[1],[Defines whether we have the RISC-V Spike Simulator library])])
  AM_CONDITIONAL([USE_STAKE], [test "x$sst_check_stake_happy" = "xyes"])
  AC_DEFINE_UNQUOTED([STAKE_LIBDIR], ["$STAKE_LIBDIR"], [Path to RISCV tools installation])

  AC_MSG_CHECKING([for RISC-V Spike simulator library])
  AC_MSG_RESULT([$sst_check_stake_happy])
  AS_IF([test "$sst_check_stake_happy" = "no" -a ! -z "$with_stake" -a "$with_stake" != "no"], [$3])
  AS_IF([test "$sst_check_stake_happy" = "yes"], [$1], [$2])
])
