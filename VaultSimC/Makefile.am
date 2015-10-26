# -*- Makefile -*-
#
#


compdir = $(pkglibdir)
comp_LTLIBRARIES = libVaultSimC.la

libVaultSimC_la_SOURCES = \
  DRAMSim2/AddressMapping.cpp \
  DRAMSim2/BankState.cpp \
  DRAMSim2/ClockDomain.cpp \
  DRAMSim2/IniReader.cpp \
  DRAMSim2/MemorySystem.cpp \
  DRAMSim2/Rank.cpp \
  DRAMSim2/TraceBasedSim.cpp \
  DRAMSim2/Bank.cpp \
  DRAMSim2/BusPacket.cpp \
  DRAMSim2/CommandQueue.cpp \
  DRAMSim2/MemoryController.cpp \
  DRAMSim2/MultiChannelMemorySystem.cpp \
  DRAMSim2/SimulatorObject.cpp \
  DRAMSim2/Transaction.cpp \
  libVaultSimGen.cpp \
  logicLayer.h \
  logicLayer.cpp \
  VaultSimC.cpp \
  VaultSimC.h \
  Vault.cpp \
  Vault.h \
  globals.h

libVaultSimC_la_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS) -DUSE_VAULTSIM_HMC -DNO_STORAGE -DUSING_SST
libVaultSimC_la_LDFLAGS = -module -avoid-version
