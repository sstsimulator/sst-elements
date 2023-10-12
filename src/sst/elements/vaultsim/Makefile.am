# -*- Makefile -*-
#
#

AM_CPPFLAGS += \
	$(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src

EXTRA_DIST = \
	tests/testsuite_default_VaultSim.py \
	tests/sdl1.ref \
	tests/sdl2.ref \
	tests/sdl3.xml.ref \
	tests/sdl1.py \
	tests/sdl2.py \
	tests/sdl3.py \
	tests/stream-8coreVault_1.6GHz-M5.xml \
	tests/refFiles/test_VaultSim_sdl1.out \
	tests/refFiles/test_VaultSim_sdl2.out

compdir = $(pkglibdir)
comp_LTLIBRARIES = libvaultsim.la

libvaultsim_la_SOURCES = \
	logicLayer.h \
	logicLayer.cc \
	cpu.h \
	cpu.cpp \
	app.cpp \
	vaultsim.cc \
	vaultsim.h \
	memReqEvent.h \
	globals.h

libvaultsim_la_LDFLAGS = -module -avoid-version

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     vaultsim=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      vaultsim=$(abs_srcdir)/tests
