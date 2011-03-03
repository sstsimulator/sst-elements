# -*- Makefile -*-
#
#

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libmesmthi.la
libmesmthi_la_SOURCES = mesmthi.cc mesmthi_model.cc mesmthi_model.h mesmthi.h \
                        aux/des.h aux/cont_proc.h
libmesmthi_la_LDFLAGS = -module -avoid-version -ldl -lqsim
