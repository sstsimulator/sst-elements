AC_DEFUN([SST_CHECK_SPINLOCK], [

AC_ARG_ENABLE([spinlock],
  [AS_HELP_STRING([--(dis|en)able-spinlock],
    [enable spin locks for more efficient thread barriers [default=disable]])],
  [sst_check_spinlock_happy="yes"],
  [sst_check_spinlock_happy="no"])
if test "X$sst_check_spinlock_happy" = "Xyes"; then
  if test "X$sst_check_osx_happy" = "Xno"; then
    AC_DEFINE_UNQUOTED([USE_SPINLOCK], 1, "Whether to use spin locks for more efficient thread barriers")
  else
    AC_MSG_ERROR([--enable-spinlock not available on darwin])
  fi
fi

])
