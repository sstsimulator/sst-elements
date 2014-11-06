
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
	xml/trace.xml \
	xml/trace.py \
	xml/trace-withdramsim.xml \
	xml/trace-withdramsim.py

libprospero_la_LDFLAGS = -module -avoid-version

if USE_LIBZ
libprospero_la_LIBADD = -lz

libprospero_la_SOURCES += \
	prosbingzreader.h \
	prosbingzreader.cc
endif
