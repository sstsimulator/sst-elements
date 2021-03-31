dnl -*- Autoconf -*-
dnl vim:ft=config
dnl

AC_DEFUN([SST_memHierarchy_CONFIG], [
	mh_happy="yes"

  # Use global Ramulator check
  SST_CHECK_RAMULATOR([],[],[AC_MSG_ERROR([Ramulator requested but could not be found])])

  # Use global DRAMSim check
  SST_CHECK_DRAMSIM([],[],[AC_MSG_ERROR([DRAMSim requested but could not be found])])

  # Use global DRAMSim3 check
  SST_CHECK_DRAMSIM3([],[],[AC_MSG_ERROR([DRAMSim3 requested but could not be found])])

  # Use global HBMDRAMSim check
  SST_CHECK_HBMDRAMSIM([],[],[AC_MSG_ERROR([HBM DRAMSim requested but could not be found])])

  # Use global HybridSim check
  SST_CHECK_HYBRIDSIM([],[],[AC_MSG_ERROR([HybridSim requested but could not be found])])

  # Use GOBLIN HMC Sim
  SST_CHECK_GOBLIN_HMCSIM([],[],[AC_MSG_ERROR([GOBLIN HMC Sim requested but could not be found])])

  # Use NVDIMM Sim
  SST_CHECK_NVDIMMSIM([],[],[AC_MSG_ERROR([NVDIMMSim requested but could not be found])])

  # Use FlashDIMMSim
  SST_CHECK_FDSIM([],[],[AC_MSG_ERROR([FlashDIMMSim requested but could not be found])])

  # Use LIBZ
  SST_CHECK_LIBZ()

  AS_IF([test "$mh_happy" = "yes"], [$1], [$2])
])
