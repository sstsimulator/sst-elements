# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libCramSim.la
libCramSim_la_SOURCES = \
	CramSim.cpp \
	c_TraceFileReader.hpp \
	c_TraceFileReader.cpp \
	c_Dimm.hpp \
	c_Dimm.cpp \
	c_Bank.hpp \
	c_Bank.cpp \
	c_TxnConverter.hpp \
	c_TxnConverter.cpp \
	c_Transaction.hpp \
	c_Transaction.cpp \
	c_AddressHasher.hpp \
	c_AddressHasher.cpp \
	c_HashedAddress.hpp \
	c_HashedAddress.cpp \
	c_DeviceDriver.hpp \
	c_DeviceDriver.cpp \
	c_Controller.cpp \
	c_Controller.hpp \
	c_BankCommand.hpp \
	c_BankCommand.cpp \
	c_BankState.hpp \
	c_BankStateActivating.hpp \
	c_BankStateActivating.cpp \
	c_BankStateActive.hpp \
	c_BankStateActive.cpp \
	c_BankStateIdle.hpp \
	c_BankStateIdle.cpp \
	c_BankStatePrecharge.hpp \
	c_BankStatePrecharge.cpp \
	c_BankStateRead.hpp \
	c_BankStateRead.cpp \
	c_BankStateReadA.hpp \
	c_BankStateReadA.cpp \
	c_BankStateRefresh.hpp \
	c_BankStateRefresh.cpp \
	c_BankStateWrite.hpp \
	c_BankStateWrite.cpp \
	c_BankStateWriteA.hpp \
	c_BankStateWriteA.cpp \
	c_Rank.hpp \
	c_Rank.cpp \
	c_Channel.hpp \
	c_Channel.cpp \
	c_BankGroup.hpp \
	c_BankGroup.cpp \
	c_BankInfo.hpp \
	c_BankInfo.cpp \
	c_TxnReqEvent.hpp \
	c_TxnResEvent.hpp \
	c_CmdPtrPkgEvent.hpp \
	c_TokenChgEvent.hpp \
	c_CmdReqEvent.hpp \
	c_CmdResEvent.hpp \
	c_MemhBridge.hpp \
	c_MemhBridge.cpp \
	c_TxnScheduler.cpp \
	c_TxnScheduler.hpp \
	c_CmdScheduler.cpp \
	c_CmdScheduler.hpp \
	c_TxnDispatcher.hpp \
	c_TxnDispatcher.cpp \
	c_TxnGen.hpp \
	c_TxnGen.cpp \
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
	tests/VeriMem/test_verimem1.py \
	tests/test_txngen.py \
	tests/test_txntrace.py

libCramSim_la_LDFLAGS = -module -avoid-version

##########################################################################
##########################################################################
##########################################################################
