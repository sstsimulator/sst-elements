# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src

compdir = $(pkglibdir)
comp_LTLIBRARIES = librdmaNic.la
librdmaNic_la_SOURCES = \
	rdmaNicBarrier.h \
	rdmaNic.cc \
	rdmaNic.h \
	rdmaNicCmds.h \
	rdmaNicHostInterface.h \
	rdmaNicMemRequestQ.h \
	rdmaNicNetworkQueue.cc \
	rdmaNicNetworkQueue.h \
	rdmaNicRecvEngine.cc \
	rdmaNicRecvEngine.h \
	rdmaNicSendEngine.cc \
	rdmaNicSendEngine.h \
	rdmaNicSendEntry.h \
	rdmaNicMemRequest.h \
	rdmaNicNetworkEvent.h \
	rdmaNicTree.h

EXTRA_DIST = \
	tests/testsuite_default_rdmaNic.py \
	tests/runVanadis.py \
	tests/oneRtrV3.py \
	tests/oneRtrV4.py \
	tests/vanadisBlock.py 

sstdir = $(includedir)/sst/elements/rdmaNic
nobase_sst_HEADERS = \
	rdmaNicBarrier.h \
	rdmaNic.h \
	rdmaNicCmds.h \
	rdmaNicHostInterface.h \
	rdmaNicMemRequestQ.h \
	rdmaNicNetworkQueue.h \
	rdmaNicRecvEngine.h \
	rdmaNicSendEngine.h \
	rdmaNicSendEntry.h \
	rdmaNicMemRequest.h \
	rdmaNicNetworkEvent.h \
	rdmaNicTree.h

librdmaNic_la_LDFLAGS = -module -avoid-version
librdmaNic_la_LIBADD =

AM_CPPFLAGS += $(HMC_FLAG)

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     rdmaNic=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      rdmaNic=$(abs_srcdir)/tests

