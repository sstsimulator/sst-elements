dnl -*- Autoconf -*-

AC_DEFUN([SST_ember_CONFIG], [
  sst_check_ember="yes"

  AC_ARG_ENABLE([ember-contexts],
	AS_HELP_STRING([--enable-ember-contexts], [Enable context switching in Ember]))

  AM_CONDITIONAL([USE_EMBER_CONTEXTS], [test "x$enable_ember_contexts" = "xyes"])
  AS_IF([test "x$enable_ember_contexts" = "xyes"], 
	[AC_DEFINE([HAVE_EMBER_CONTEXTS], [1], [Use context switching code in Ember])])

  AS_IF([test "$sst_check_ember" = "yes"], [$1], [$2])
])
