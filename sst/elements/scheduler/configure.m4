AC_DEFUN([SST_scheduler_CONFIG], [
  SST_CHECK_BOOST_FILESYSTEM
  AX_BOOST_THREAD
  AS_IF([test "x$ax_cv_boost_thread" = "xyes"], [$1], [$2])
])
