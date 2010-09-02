# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libbit_bucket.la
libbit_bucket_la_SOURCES = \
		bit_bucket.h \
		bit_bucket.cc
libbit_bucket_la_LDFLAGS = -module -avoid-version
