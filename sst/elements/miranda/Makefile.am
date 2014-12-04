
AM_CPPFLAGS = \
	$(BOOST_CPPFLAGS) \
	$(MPI_CPPFLAGS) 

compdir = $(pkglibdir)
comp_LTLIBRARIES = libmiranda.la

libmiranda_la_SOURCES = \
	miranda.cc \
	mirandaGenerator.h \
	mirandaCPU.cc \
	mirandaCPU.h	\
	generators/singlestream.h \
	generators/singlestream.cc \
	generators/randomgen.h \
	generators/randomgen.cc

EXTRA_DIST = \
	tests/randomgen.py \
	tests/singlestream.py

libmiranda_la_LDFLAGS = -module -avoid-version
