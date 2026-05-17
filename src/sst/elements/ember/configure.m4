dnl -*- Autoconf -*-

AC_DEFUN([SST_ember_CONFIG], [
  sst_check_ember="yes"

  SST_CHECK_OTF2([sst_check_ember_otf2="yes"], [sst_check_ember_otf2="no"])
  AM_CONDITIONAL([EMBER_HAVE_OTF2], [test "x$sst_check_ember_otf2" = "xyes"])

  AS_IF([test "$sst_check_ember" = "yes"], [$1], [$2])
])
