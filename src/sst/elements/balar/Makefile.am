AM_CPPFLAGS = \
	$(MPIT_CPPFLAGS)

AM_LDFLAGS = -lm
compdir = $(pkglibdir)
comp_LTLIBRARIES = libbalar.la

libbalar_la_SOURCES = \
	mempool.h \
	mempool.cpp \
	balar.cc \
	balar.h

libbalar_la_CPPFLAGS = \
	-I$(top_srcdir)/src \
	$(SST_CXX0X_FLAGS) -fPIC -Wall \
	$(MPI_CPPFLAGS) \
	$(CUDA_CPPFLAGS)

libbalar_la_LDFLAGS = \
	-avoid-version

libbalar_la_LIBADD = \
	$(SST_SYSTEMC_LIB) \
	-L$(GPGPUSIM_LIBDIR) \
	$(GPGPUSIM_LIB)

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE balar=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS  balar=$(abs_srcdir)/tests
