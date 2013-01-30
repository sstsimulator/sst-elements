dnl -*- Autoconf -*-

AC_DEFUN([SST_zesto_CONFIG], [
  AC_ARG_WITH([qsim],
    [AS_HELP_STRING([--with-qsim@<:@=DIR@:>@],
      [Use QSim package installed in optionally specified DIR])])
  AC_ARG_ENABLE([zesto], [AS_HELP_STRING([--enable-zesto], [Enable the Zesto processor component])],
        [], [enable_zesto="no"])

  happy="yes"
  AS_IF([test "$with_qsim" = "no"], [happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"

  AS_IF([test ! -z "$with_qsim" -a "$with_qsim" != "yes"],
    [QSIM_CPPFLAGS="-I$with_qsim -DUSE_QSIM"
     CPPFLAGS="$QSIM_CPPFLAGS $CPPFLAGS"
     QSIM_LDFLAGS="-L$with_qsim"
     LDFLAGS="$QSIM_LDFLAGS $LDFLAGS"],
    [QSIM_CPPFLAGS=
     QSIM_LDFLAGS=])

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([qsim-client.h], [], [happy="no"])
  LIBS="$LIBS -ldl"
  AC_CHECK_LIB([qsim], [qsim_present], 
    [QSIM_LIB="-lqsim -lqsim-client -ldl"], [happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"
  BUILD_ZESTO=""

  AS_IF( [test "$enable_zesto" = "yes"],
        [AS_IF([test "$happy" = "no"], AC_MSG_FAILURE(["Zesto enabled by user but required libraries cannot be found."]),
                 [AC_MSG_NOTICE(["Zesto build enabled by user."])])],
        [AC_MSG_NOTICE(["Zesto has not been enabled by user, build is disabled"])] )

  AC_SUBST([QSIM_CPPFLAGS])
  AC_SUBST([QSIM_LDFLAGS])
  AC_SUBST([QSIM_LIB])
  AM_CONDITIONAL([BUILD_ZESTO], [test "$enable_zesto" = "yes"])

  AS_IF([test "$happy" = "yes"],
    [QSIM_CPPFLAGS="$QSIM_CPPFLAGS -DUSE_QSIM"]
  )

  AS_IF([test "$happy" = "yes"], [$1], [$2])
])
