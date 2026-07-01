dnl -*- Autoconf -*-
dnl vim:ft=config
dnl

AC_DEFUN([SST_memHierarchy_CONFIG], [
	mh_happy="yes"

  # Use global Ramulator check
  SST_CHECK_RAMULATOR([],[],[AC_MSG_ERROR([Ramulator requested but could not be found])])

  # Use global Ramulator2 check
  SST_CHECK_RAMULATOR2([],[],[AC_MSG_ERROR([Ramulator2 requested but could not be found])])

  # Use GOBLIN HMC Sim
  SST_CHECK_GOBLIN_HMCSIM([],[],[AC_MSG_ERROR([GOBLIN HMC Sim requested but could not be found])])

  # Use LIBZ
  SST_CHECK_LIBZ()

  AS_IF([test "$mh_happy" = "yes"], [$1], [$2])
])
