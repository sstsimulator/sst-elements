dnl -*- Autoconf -*-

AC_DEFUN([SST_zodiac_CONFIG], [

  zodiac_happy="yes"

  SST_CHECK_OTF([have_zodiac_otf=1],
	[have_zodiac_otf=0],
	[AC_MSG_ERROR([Open Trace Format was requested but was not found])])
  SST_CHECK_DUMPI([have_zodiac_dumpi=1],
	[have_zodiac_dumpi=0],
	[AC_MSG_ERROR([DUMPI Trace Format was requested but was not found])])

  AS_IF([test "$have_zodiac_otf" = 1],
	[AC_DEFINE([HAVE_ZODIAC_OTF], [1], [Define if you have an OTF compatible library.])])
  AS_IF([test "$have_zodiac_dumpi" = 1],
	[AC_DEFINE([HAVE_ZODIAC_DUMPI], [1], [Define if you have an UNDUMPI compatible library.])])

  AS_IF([test "$zodiac_happy" = "yes"], [$1], [$2])

])
