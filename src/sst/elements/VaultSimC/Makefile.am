# -*- Makefile -*-
#
#


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
comp_LTLIBRARIES = libVaultSimC.la

libVaultSimC_la_SOURCES = \
	logicLayer.h \
	logicLayer.cpp \
	cpu.h \
	cpu.cpp \
	app.cpp \
	VaultSimC.cpp \
	VaultSimC.h \
	memReqEvent.h \
	vaultGlobals.h

libVaultSimC_la_CPPFLAGS = $(MPI_CPPFLAGS) \
	$(LIBPHX_CPPFLAGS) -I$(top_srcdir)/src
libVaultSimC_la_LDFLAGS = -module -avoid-version \
	$(LIBPHX_LDFLAGS) $(LIBPHX_LIBDIR)
libVaultSimC_la_LIBADD = $(LIBPHX_LIB)

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     VaultSimC=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      VaultSimC=$(abs_srcdir)/tests
