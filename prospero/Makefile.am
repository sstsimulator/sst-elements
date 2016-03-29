
AM_CPPFLAGS = \
        $(BOOST_CPPFLAGS) \
        $(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libprospero.la

libprospero_la_SOURCES = \
	libprospero.cc \
        proscpu.h \
        proscpu.cc \
	prosreader.h \
	prostextreader.h \
	prostextreader.cc \
	prosbinaryreader.h \
	prosbinaryreader.cc \
	prosmemmgr.h \
	prosmemmgr.cc

EXTRA_DIST = \
        tests/array/trace-binary.py \
        tests/array/trace-binary-withdramsim.py \
        tests/array/trace-compressed.py \
        tests/array/trace-compressed-withdramsim.py \
        tests/array/trace-text.py \
        tests/array/trace-text-withdramsim.py \
        tests/array/trace-common.py \
        tests/array/array.c \
        tests/array/Makefile \
        tracetool/Makefile \
        tracetool/Makefile.osx \
        tracetool/sstmemtrace.cc \
        tracetool/api/Makefile \
        tracetool/api/Makefile.osx \
        tracetool/api/prospero.c \
        tracetool/api/prospero.h

libprospero_la_LDFLAGS = -module -avoid-version

bin_PROGRAMS = sst-prospero-trace

sst_prospero_trace_SOURCES = runprosperotrace.cc

if USE_LIBZ
libprospero_la_LIBADD = -lz

libprospero_la_SOURCES += \
	prosbingzreader.h \
	prosbingzreader.cc
endif

if SST_COMPILE_OSX

libexec_PROGRAMS += fesimple.dylib
prosperotrace_dylib_SOURCES = tracetool/sstmemtrace.cc

else

libexec_PROGRAMS += fesimple.so
prosperotrace_so_SOURCES = tracetool/sstmemtrace.cc

endif
