# -*- Makefile -*-
#
#

AM_CPPFLAGS = -g

compdir = $(pkglibdir)
comp_LTLIBRARIES = libVaultSimC.la

libVaultSimC_la_SOURCES = \
    logicLayer.h \
    logicLayer.cpp \
    VaultSimC.cpp \
    VaultSimC.h \
    Vault.cpp \
    Vault.h \
    globals.h \
    libVaultSimGen.cpp

libVaultSimC_la_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS) -DUSE_VAULTSIM_HMC 
libVaultSimC_la_LDFLAGS = -module -avoid-version
libVaultSimC_la_LIBADD = 

if HAVE_DRAMSIM
libVaultSimC_la_CPPFLAGS += $(DRAMSIM_CPPFLAGS) -DHAVE_LIBDRAMSIM
libVaultSimC_la_LDFLAGS += $(DRAMSIM_LDFLAGS)
libVaultSimC_la_LIBADD += $(DRAMSIM_LIB)
endif
