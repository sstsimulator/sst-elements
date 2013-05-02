dnl -*- Autoconf -*-

AC_DEFUN([SST_ariel_CONFIG], [
  AC_ARG_ENABLE([ariel], [AS_HELP_STRING([--enable-ariel], [Enable the Ariel tracing component])],
        [], [enable_ariel="no"])

  happy="yes"

  SST_CHECK_PINTOOL([have_pin=1],[have_pin=0],[AC_MSG_ERROR([PIN was requested but not found])])

  AS_IF( [test "$enable_ariel" = "yes"], [happy="yes"], [happy="no"] )
  AS_IF( [test "$enable_ariel" = "yes" -a "$have_pin" = "0"], [AC_MSG_ERROR(["Ariel is requested by PIN cannot be found."])],
	[AC_MSG_NOTICE(["Ariel component configure found that PIN has been located successfully."])] )

  AS_IF([test "$happy" = "yes"], [$1], [$2])
])
