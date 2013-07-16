# -*- Makefile -*-
#
#

EXTRA_DIST = 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libVaultSimC.la
libVaultSimC_la_SOURCES = \
	libVaultSimGen.cpp \
	logicLayer.h \
	logicLayer.cpp \
	cpu.h \
	cpu.cpp \
	app.cpp \
	VaultSimC.cpp \
	VaultSimC.h \
	vaultGlobals.h \
	libphx/BankState.cpp \
	libphx/BankState.h \
	libphx/BusPacket.cpp \
	libphx/BusPacket.h \
	libphx/Callback.h \
	libphx/Globals.h \
	libphx/Partition.cpp \
	libphx/Partition.h \
	libphx/Transaction.cpp \
	libphx/Transaction.h \
	libphx/Vault.cpp \
	libphx/Vault.h \
	libphx/VaultController.cpp \
	libphx/VaultController.h

libVaultSimC_la_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS) \
            -I$(top_srcdir)/sst/elements/include -Ilibphx
libVaultSimC_la_LDFLAGS = -module -avoid-version 
#libVaultSimC_la_LIBADD = $(PHXSIM_LIB)
