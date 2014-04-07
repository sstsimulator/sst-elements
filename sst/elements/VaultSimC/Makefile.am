# -*- Makefile -*-
#
#

if HAVE_LIBPHX

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
	vaultGlobals.h

libVaultSimC_la_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS) \
	$(LIBPHX_CPPFLAGS)
libVaultSimC_la_LDFLAGS = -module -avoid-version \
	$(LIBPHX_LDFLAGS) $(LIBPHX_LIBDIR) 
libVaultSimC_la_LIBADD = $(LIBPHX_LIB)

endif
