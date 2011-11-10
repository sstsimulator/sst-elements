# -*- Makefile -*-
#
#

comp_LTLIBRARIES += libbit_bucket.la
libbit_bucket_la_SOURCES = \
		components/bit_bucket/bit_bucket.h \
		components/bit_bucket/bit_bucket.cc
libbit_bucket_la_LDFLAGS = -module -avoid-version
