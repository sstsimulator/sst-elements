# -*- Makefile -*-
#
#

AM_CPPFLAGS += \
	$(MPI_CPPFLAGS)


compdir = $(pkglibdir)

# define library for this module
comp_LTLIBRARIES = libPandosProgramming.la

# add sources for this element library here
libPandosProgramming_la_SOURCES = \
	PandosNode.h \
	PandosNode.cc

# deprecated sources... I guess?
deprecated_libPandosProgramming_sources =

if !SST_ENABLE_PREVIEW_BUILD
libPandosProgramming_la_SOURCES += $(deprecated_libPandosProgramming_sources)
endif

# other stuff that should be included in the install
EXTRA_DIST = \
	tests/pandos-node.py

deprecated_EXTRA_DIST =

if !SST_ENABLE_PREVIEW_BUILD
EXTRA_DIST += $(deprecated_EXTRA_DIST)
endif

libPandosProgramming_la_LDFLAGS = -module -avoid-version
libPandosProgramming_la_LIBADD =
if HAVE_PANDO_RUNTIME
libPandosProgramming_la_LDFLAGS += $(PANDO_RUNTIME_LDFLAGS)
libPandosProgramming_la_LIBADD += $(PANDO_RUNTIME_LIB)
AM_CPPFLAGS += $(PANDO_RUNTIME_CPPFLAGS)
endif

install-exec-hook:
	$(SST_REGISTER_TOOL) PANDO_RUNTIME LIBDIR=$(PANDO_RUNTIME_LIBDIR)
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE PandosProgramming=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS  PandosProgramming=$(abs_srcdir)/tests
