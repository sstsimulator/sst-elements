# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libCramSim.la
libCramSim_la_SOURCES = \
	CramSim.cpp \
	c_TxnGenSeq.hpp \
	c_TxnGenSeq.cpp \
	c_TxnGenRand.hpp \
	c_TxnGenRand.cpp \
	c_TracefileReader.hpp \
	c_TracefileReader.cpp \
	c_DramSimTraceReader.hpp \
	c_DramSimTraceReader.cpp \
	c_USimmTraceReader.hpp \
	c_USimmTraceReader.cpp \
	c_TxnDriver.hpp \
	c_TxnDriver.cpp \
	c_TxnUnit.hpp \
	c_TxnUnit.cpp \
	c_CmdDriver.hpp \
	c_CmdDriver.cpp \
	c_CmdUnit.hpp \
	c_CmdUnit.cpp \
	c_BankReceiver.hpp \
	c_BankReceiver.cpp \
	c_Dimm.hpp \
	c_Dimm.cpp \
	c_Bank.hpp \
	c_Bank.cpp \
	c_Transaction.hpp \
	c_Transaction.cpp \
	c_TransactionToCommands.hpp \
	c_TransactionToCommands.cpp \
	c_AddressHasher.hpp \
	c_AddressHasher.cpp \
	c_HashedAddress.hpp \
	c_HashedAddress.cpp \
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
	memReqEvent.hpp 

EXTRA_DIST = \
	README \
	ddr4_verimem.cfg \
	tests/VeriMem/test_verimem1.py \
	tests/test_txngenrand1.py \
	tests/test_txntrace3.py \
	tests/test_txntrace1.py \
	tests/test_txntrace2.py \
	tests/test_txngenseq1.py \
	tests/test_txngenseq2.py \
	tests/test_txngenrand2.py \
	tests/test_txntrace4.py

libCramSim_la_LDFLAGS = -module -avoid-version

##########################################################################
##########################################################################
##########################################################################
