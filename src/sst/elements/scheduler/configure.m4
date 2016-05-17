AC_DEFUN([SST_scheduler_CONFIG], [
  sst_scheduler_happy="yes"
  SST_CHECK_METIS([],[])

  #check for GLPK
  SST_CHECK_GLPK([],[],[AC_MSG_ERROR([GLPK requested but could not be found])])

  AS_IF([test "x$sst_scheduler_happy" = "xyes"], [$1], [$2])
])
