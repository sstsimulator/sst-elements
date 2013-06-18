dnl -*- Autoconf -*-

AC_DEFUN([SST_zodiac_CONFIG], [

  SST_CHECK_OTF([have_otf=1],
	[have_otf=0],
	[AC_MSG_ERROR([Open Trace Format was requested but was not found])])

  AS_IF([test "$have_otf" = 1],
	[AC_DEFINE([have_otf], [1], [Define if you have an OTF compatible library.])])

  AS_IF([test "$have_otf" = "1"], [$1], [$2])

])
