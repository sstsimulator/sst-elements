AC_DEFUN([SST_CHECK_QSIM],[

  AC_ARG_WITH([qsim],
    [AS_HELP_STRING([--with-qsim@<:@=DIR@:>@],
      [Use QSim installed at optionally-specified prefix DIR])])

  sst_check_qsim_happy="yes"

  AS_IF([test "$with_qsim" = "no"], [sst_check_qsim_happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"

  AS_IF([test ! -z "$with_qsim" -a "$with_qsim" != "yes"],
    [QSIM_CPPFLAGS="-I$with_qsim/include"
     CPPFLAGS="$QSIM_CPPFLAGS $CPPFLAGS"
     QSIM_LDFLAGS="-L$with_qsim/lib"
     LDFLAGS="$QSIM_LDFLAGS $LDFLAGS"
     QSIM_LIBDIR="$with_qsim/lib"],
    [QSIM_CPPFLAGS=
     QSIM_LDFLAGS=
     QSIM_LIBDIR=])

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([qsim.h], [], [sst_check_qsim_happy="no"])
  AC_CHECK_LIB([qsim], [qsim_present], 
    [QSIM_LIBS="-lqsim"], [sst_check_qsim_happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"

  AC_SUBST([QSIM_CPPFLAGS])
  AC_SUBST([QSIM_LDFLAGS])
  AC_SUBST([QSIM_LIBS])
  AC_SUBST([QSIM_LIBDIR])
  AS_IF([test "$sst_check_qsim_happy" = "yes"],
        [AC_DEFINE([HAVE_QSIM], [1], [Set to 1 if QSim was found])])
  AC_DEFINE_UNQUOTED([QSIM_LIBDIR], ["$QSIM_LIBDIR"], [Path to QSim library])

  AS_IF([test "$sst_check_qsim_happy" = "yes"], [$1], [$2])
])
