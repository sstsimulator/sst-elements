# -*- Makefile -*-
#
#

AM_CPPFLAGS += \
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
	tests/memory.py \
	tests/node.py \
	tests/rdmaNic.py \
	tests/runVanadis.py \
	tests/vanadisBlock.py \
	tests/testsuite_default_rdmaNic.py \
	tests/vanadisOS.py \
	tests/msglen.txt \
	tests/app/rdma/Makefile \
	tests/app/rdma/barrier.c \
	tests/app/rdma/incast.c \
	tests/app/rdma/incast-v2.c \
	tests/app/rdma/include/base.h \
	tests/app/rdma/include/rdma.h \
	tests/app/rdma/msg.c \
	tests/app/rdma/riscv64/msg.err.gold \
	tests/app/rdma/riscv64/msg.out.gold \
	tests/app/rdma/riscv64/msg.stderr-node0.cpu0.os.gold \
	tests/app/rdma/riscv64/msg.stderr-node1.cpu0.os.gold \
	tests/app/rdma/riscv64/msg.stdout-node0.cpu0.os.gold \
	tests/app/rdma/riscv64/msg.stdout-node1.cpu0.os.gold \
	tests/app/rdma/write.c \
	tests/app/rdma/src/base.c \
	tests/app/rdma/src/rdma.c \
	tests/app/mpi/configure_rdma \
	tests/app/mpi/patch.diff \
	tests/app/mpi/runVanadis \
	tests/app/mpi/riscv64/IMB-MPI1 \
	tests/app/mpi/riscv64/IMB-MPI1.err.gold \
	tests/app/mpi/riscv64/IMB-MPI1.out.gold \
	tests/app/mpi/riscv64/IMB-MPI1.stderr-node0.cpu0.os.gold \
	tests/app/mpi/riscv64/IMB-MPI1.stderr-node1.cpu0.os.gold \
	tests/app/mpi/riscv64/IMB-MPI1.stdout-node0.cpu0.os.gold \
	tests/app/mpi/riscv64/IMB-MPI1.stdout-node1.cpu0.os.gold \
	tests/app/mpi/README

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

