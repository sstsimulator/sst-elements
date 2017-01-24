AC_DEFUN([SST_CHECK_GOBLIN_HMCSIM],
[
  sst_check_goblin_hmcsim_happy="yes"

  AC_ARG_WITH([goblin-hmcsim],
    [AS_HELP_STRING([--with-goblin-hmcsim@<:@=DIR@:>@],
      [Use GOBLIN HMC Simulator installed in optionally specified DIR])])

  AS_IF([test "$with_goblin_hmcsim" = "no"], [sst_check_goblin_hmcsim_happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"

  AS_IF([test "$sst_check_goblin_hmcsim_happy" = "yes"], [
    AS_IF([test ! -z "$with_goblin_hmcsim" -a "$with_goblin_hmcsim" != "yes"],
      [GOBLIN_HMCSIM_CPPFLAGS="-I$with_goblin_hmcsim/include"
       CPPFLAGS="$GOBLIN_HMCSIM_CPPFLAGS $CPPFLAGS"
       GOBLIN_HMCSIM_LDFLAGS="-L$with_goblin_hmcsim/lib -L$with_goblin_hmcsim"
       GOBLIN_HMCSIM_LIB="-lhmcsim",
       LDFLAGS="$GOBLIN_HMCSIM_LDFLAGS $LDFLAGS"
       GOBLIN_HMCSIM_LIBDIR="$with_goblin_hmcsim"],
      [GOBLIN_HMCSIM_CPPFLAGS=
       GOBLIN_HMCSIM_LDFLAGS=
       GOBLIN_HMCSIM_LIB=
       GOBLIN_HMCSIM_LIBDIR=])])
  
  AC_LANG_PUSH([C++])
  AC_CHECK_HEADER([hmc_sim.h], [], [sst_check_goblin_hmcsim_happy="no"])
  AC_LANG_POP([C++])

  AC_CHECK_LIB([hmcsim], [hmcsim_clock],
    [GOBLIN_HMCSIM_LIB="-lhmcsim"], [sst_check_goblin_hmcsim_happy="no"])

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"

  AC_SUBST([GOBLIN_HMCSIM_CPPFLAGS])
  AC_SUBST([GOBLIN_HMCSIM_LDFLAGS])
  AC_SUBST([GOBLIN_HMCSIM_LIB])
  AC_SUBST([GOBLIN_HMCSIM_LIBDIR])
  AS_IF([test "x$sst_check_goblin_hmcsim_happy" = "xyes"], [AC_DEFINE([HAVE_GOBLIN_HMCSIM],[1],[Defines whether we have the GOBLIN HMC Simulator library])])
  AM_CONDITIONAL([USE_GOBLIN_HMCSIM], [test "x$sst_check_goblin_hmcsim_happy" = "xyes"])
  AC_DEFINE_UNQUOTED([GOBLIN_HMCSIM_LIBDIR], ["$GOBLIN_HMCSIM_LIBDIR"], [Path to Goblin HMCSIM library])

  AC_MSG_CHECKING([for GOBLIN HMC Simulator library])
  AC_MSG_RESULT([$sst_check_goblin_hmcsim_happy])
  AS_IF([test "$sst_check_goblin_hmcsim_happy" = "no" -a ! -z "$with_goblin_hmcsim" -a "$with_goblin_hmcsim" != "no"], [$3])
  AS_IF([test "$sst_check_goblin_hmcsim_happy" = "yes"], [$1], [$2])
])
