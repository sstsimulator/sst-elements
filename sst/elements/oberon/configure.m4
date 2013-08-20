dnl -*- Autoconf -*-

AC_DEFUN([SST_oberon_CONFIG], [
  AC_ARG_ENABLE([oberon], [AS_HELP_STRING([--enable-oberon], [Enable the Oberon state machine scripting component])], 
	[], [enable_oberon="no"])
  sst_oberon_happy="yes"
  
  AS_IF( [test "$enable_oberon" != "yes"], [sst_oberon_happy="no"] )
  AS_IF([test "$sst_oberon_happy" = "yes"], [$1], [$2])

])
