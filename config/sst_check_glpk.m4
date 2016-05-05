
AC_DEFUN([SST_CHECK_GLPK], [
  AC_ARG_WITH([glpk],
    [AS_HELP_STRING([--with-glpk@<:@=DIR@:>@],
      [Use GNU GLPK installed in optionally specified DIR])])

  sst_check_glpk_happy="yes"
  AS_IF([test "$with_glpk" = "no"], [sst_check_glpk_happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"

  AS_IF([test ! -z "$with_glpk" -a "$with_glpk" != "yes"],
    [GLPK_CPPFLAGS="-I$with_glpk/include -DHAVE_GLPK=1"
     CPPFLAGS="$GLPK_CPPFLAGS $CPPFLAGS"
     GLPK_LDFLAGS="-L$with_glpk/lib"
     GLPK_LIBDIR="$with_glpk/lib"
     LDFLAGS="$GLPK_LDFLAGS $LDFLAGS"],
    [GLPK_CPPFLAGS=
     GLPK_LDFLAGS=
     GLPK_LIBDIR=])

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([glpk.h], [], [sst_check_glpk_happy="no"])
  AC_CHECK_LIB([glpk], [glp_create_prob],
    [GLPK_LIB="-lglpk"], [sst_check_glpk_happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"

  AC_SUBST([GLPK_CPPFLAGS])
  AC_SUBST([GLPK_LDFLAGS])
  AC_SUBST([GLPK_LIB])
  AC_SUBST([GLPK_LIBDIR])
  AM_CONDITIONAL([HAVE_GLPK], [test "$sst_check_glpk_happy" = "yes"])
  AS_IF([test "$sst_check_glpk_happy" = "yes"],
        [AC_DEFINE([HAVE_GLPK], [1], [Set to 1 if GLPK was found])])
  AC_DEFINE_UNQUOTED([GLPK_LIBDIR], ["$GLPK_LIBDIR"], [Path to GLPK library])

  AS_IF([test "$sst_check_glpk_happy" = "yes"], [$1], [$2])
])
