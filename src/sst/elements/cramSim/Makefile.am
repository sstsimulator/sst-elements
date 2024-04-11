# -*- Makefile -*-
#
#

AM_CPPFLAGS += \
	$(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libcramSim.la
libcramSim_la_SOURCES = \
	cramSim.cc \
	c_TraceFileReader.hpp \
	c_TraceFileReader.cc \
	c_Dimm.hpp \
	c_Dimm.cc \
	c_Bank.hpp \
	c_Bank.cc \
	c_TxnConverter.hpp \
	c_TxnConverter.cc \
	c_Transaction.hpp \
	c_Transaction.cc \
	c_AddressHasher.hpp \
	c_AddressHasher.cc \
	c_HashedAddress.hpp \
	c_HashedAddress.cc \
	c_DeviceDriver.hpp \
	c_DeviceDriver.cc \
	c_Controller.cc \
	c_Controller.hpp \
	c_BankCommand.hpp \
	c_BankCommand.cc \
	c_BankState.hpp \
	c_BankStateActivating.hpp \
	c_BankStateActivating.cc \
	c_BankStateActive.hpp \
	c_BankStateActive.cc \
	c_BankStateIdle.hpp \
	c_BankStateIdle.cc \
	c_BankStatePrecharge.hpp \
	c_BankStatePrecharge.cc \
	c_BankStateRead.hpp \
	c_BankStateRead.cc \
	c_BankStateReadA.hpp \
	c_BankStateReadA.cc \
	c_BankStateRefresh.hpp \
	c_BankStateRefresh.cc \
	c_BankStateWrite.hpp \
	c_BankStateWrite.cc \
	c_BankStateWriteA.hpp \
	c_BankStateWriteA.cc \
	c_Rank.hpp \
	c_Rank.cc \
	c_Channel.hpp \
	c_Channel.cc \
	c_BankGroup.hpp \
	c_BankGroup.cc \
	c_BankInfo.hpp \
	c_BankInfo.cc \
	c_TxnReqEvent.hpp \
	c_TxnResEvent.hpp \
	c_CmdPtrPkgEvent.hpp \
	c_TokenChgEvent.hpp \
	c_CmdReqEvent.hpp \
	c_CmdResEvent.hpp \
	c_MemhBridge.hpp \
	c_MemhBridge.cc \
	c_TxnScheduler.cc \
	c_TxnScheduler.hpp \
	c_CmdScheduler.cc \
	c_CmdScheduler.hpp \
	c_TxnDispatcher.hpp \
	c_TxnDispatcher.cc \
	c_TxnGen.hpp \
	c_TxnGen.cc \
	memReqEvent.hpp

EXTRA_DIST = \
	README \
	ddr3.cfg \
	ddr4.cfg \
	ddr4_2400.cfg \
	ddr4_2667.cfg \
	ddr4_3200.cfg \
	ddr4_verimem.cfg \
	ddr4_verimem_openbank.cfg \
	hbm_legacy_4h.cfg \
	hbm_legacy_4h_sbr.cfg \
	hbm_pseudo_4h.cfg \
	test_system.cfg \
	test_device.cfg \
	ddr3_power.cfg \
	tests/testsuite_default_cramSim.py \
	tests/VeriMem/test_verimem1.py \
	tests/test_txngen.py \
	tests/test_txntrace.py \
	tests/refFiles/test_cramSim_1_R.out \
	tests/refFiles/test_cramSim_1_RW.out \
	tests/refFiles/test_cramSim_1_W.out \
	tests/refFiles/test_cramSim_2_R.out \
	tests/refFiles/test_cramSim_2_W.out \
	tests/refFiles/test_cramSim_4_R.out \
	tests/refFiles/test_cramSim_4_W.out \
	tests/refFiles/test_cramSim_5_R.out \
	tests/refFiles/test_cramSim_5_W.out \
	tests/refFiles/test_cramSim_6_R.out \
	tests/refFiles/test_cramSim_6_W.out


libcramSim_la_LDFLAGS = -module -avoid-version

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     cramSim=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      cramSim=$(abs_srcdir)/tests

##########################################################################
##########################################################################
##########################################################################
