AC_DEFUN([SST_golem_CONFIG], [
    sst_golem_comp_happy="yes"

    SST_CHECK_NUMPY

    if test "x$sst_check_numpy_happy" = "xyes"; then
      AM_CONDITIONAL([HAVE_NUMPY], [true])
    else
      AM_CONDITIONAL([HAVE_NUMPY], [false])
    fi

    AS_IF([test "x$sst_golem_comp_happy" = "xyes"], [$1], [$2])
])
