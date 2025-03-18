# -*- Makefile -*-
#
#

AM_CPPFLAGS += \
        $(MPI_CPPFLAGS) \
        -I$(top_srcdir)/src/sst/elements/iris \
        -I$(top_srcdir)/src/sst/elements/iris/libfabric \
        -I$(top_srcdir)/src/sst/elements/iris/libfabric/include \
        -I$(top_srcdir)/src/sst/elements/iris/libfabric/prov/sumi/include \
        -I$(top_srcdir)/src/sst/elements/iris/libfabric/prov/sumi/include/rdma \
        -I$(top_srcdir)/src/sst/elements/iris/libfabric/prov/hook/include \
        -I$(top_srcdir)/src/sst/elements/mercury \
        -I$(top_srcdir)/src/sst/elements

AM_CPPFLAGS += -D_GNU_SOURCE

sumi_sources = \
  prov/sumi/src/sumi_av.cc \
  prov/sumi/src/sumi_cm.cc \
  prov/sumi/src/sumi_cntr.cc \
  prov/sumi/src/sumi_cq.cc \
  prov/sumi/src/sumi_dom.cc \
  prov/sumi/src/sumi_ep.cc \
  prov/sumi/src/sumi_eq.cc \
  prov/sumi/src/sumi_fabric.cc \
  prov/sumi/src/sumi_mr.cc \
  prov/sumi/src/sumi_poll.cc \
  prov/sumi/src/sumi_sep.cc \
  prov/sumi/src/sumi_wait.cc \
  prov/sumi/src/sumi_prov.cpp
        
sumi_headers = \
  prov/sumi/include/fi_ext_sumi.h \
  prov/sumi/include/fi_ext_sumi.h \
  prov/sumi/include/sumi_fabric.hpp \
  prov/sumi/include/sumi_prov.h \
  prov/sumi/include/sumi_av.h \
  prov/sumi/include/sumi_ep.h \
  prov/sumi/include/sumi_poll.h \
  prov/sumi/include/sumi_wait.h

sumi_rdma_headers = \
  prov/sumi/include/rdma/fi_direct.h \
  prov/sumi/include/rdma/fi_direct_atomic.h \
  prov/sumi/include/rdma/fi_direct_atomic_def.h \
  prov/sumi/include/rdma/fi_direct_cm.h \
  prov/sumi/include/rdma/fi_direct_domain.h \
  prov/sumi/include/rdma/fi_direct_endpoint.h \
  prov/sumi/include/rdma/fi_direct_eq.h \
  prov/sumi/include/rdma/fi_direct_rma.h \
  prov/sumi/include/rdma/fi_direct_tagged.h \
  prov/sumi/include/rdma/fi_direct_trigger.h

common_sources = \
  src/log.c \
  src/common.c \
  src/rbtree.c \
  src/indexer.c \
  src/perf.c \
  src/mem.c \
  src/iov.c \
  src/shared/ofi_str.c \
  src/abi_1_0.c \
  src/enosys.c \
  src/fabric.c \
  src/fasthash.c \
  src/var.c \
  src/fi_tostr.c \
  src/tree.c \
  prov/util/src/util_atomic.c \
  prov/util/src/util_attr.c \
  prov/util/src/util_av.c \
  prov/util/src/util_cq.c \
  prov/util/src/util_cntr.c \
  prov/util/src/util_domain.c \
  prov/util/src/util_ep.c \
  prov/util/src/util_pep.c \
  prov/util/src/util_eq.c \
  prov/util/src/util_fabric.c \
  prov/util/src/util_main.c \
  prov/util/src/util_poll.c \
  prov/util/src/util_wait.c \
  prov/util/src/util_buf.c \
  prov/util/src/util_mr_map.c \
  prov/util/src/util_ns.c \
  prov/util/src/util_shm.c \
  prov/util/src/util_mem_monitor.c \
  prov/util/src/util_mem_hooks.c \
  prov/util/src/util_mr_cache.c \
  prov/util/src/util_coll.c

libfabric_sources = \
  prov/hook/src/hook_cm.c \
  prov/hook/src/hook_cq.c \
  prov/hook/src/hook_xfer.c \
  prov/hook/src/hook_ep.c \
  prov/hook/src/hook_cntr.c \
  prov/hook/src/hook_av.c \
  prov/hook/src/hook.c \
  prov/hook/src/hook_wait.c \
  prov/hook/src/hook_domain.c \
  prov/hook/src/hook_eq.c

libfabric_headers = \
  libfabric_config.h \
  include/ofi.h \
  include/ofi_abi.h \
  include/ofi_atom.h \
  include/ofi_enosys.h \
  include/ofi_file.h \
  include/ofi_hook.h \
  include/ofi_indexer.h \
  include/ofi_iov.h \
  include/ofi_list.h \
  include/ofi_bitmask.h \
  include/shared/ofi_str.h \
  include/ofi_lock.h \
  include/ofi_mem.h \
  include/ofi_osd.h \
  include/ofi_proto.h \
  include/ofi_recvwin.h \
  include/ofi_rbuf.h \
  include/ofi_shm.h \
  include/ofi_signal.h \
  include/ofi_epoll.h \
  include/ofi_tree.h \
  include/ofi_util.h \
  include/ofi_atomic.h \
  include/ofi_mr.h \
  include/ofi_net.h \
  include/ofi_perf.h \
  include/ofi_coll.h \
  include/fasthash.h \
  include/rbtree.h \
  include/uthash.h \
  include/ofi_prov.h \
  include/rdma/providers/fi_log.h \
  include/rdma/providers/fi_prov.h \
  prov/hook/include/hook_prov.h

rdmainclude_headers = \
  prov/sumi/include/fi_ext_sumi.h \
  include/rdma/fabric.h \
  include/rdma/fi_atomic.h \
  include/rdma/fi_cm.h \
  include/rdma/fi_collective.h \
  include/rdma/fi_domain.h \
  include/rdma/fi_eq.h \
  include/rdma/fi_rma.h \
  include/rdma/fi_endpoint.h \
  include/rdma/fi_errno.h \
  include/rdma/fi_tagged.h \
  include/rdma/fi_trigger.h

nodist_rdmainclude_headers = \
  prov/sumi/include/rdma/fi_direct_domain.h \
  prov/sumi/include/rdma/fi_direct_endpoint.h \
  prov/sumi/include/rdma/fi_direct_tagged.h \
  prov/sumi/include/rdma/fi_direct_rma.h \
  prov/sumi/include/rdma/fi_direct_atomic_def.h \
  prov/sumi/include/rdma/fi_direct_atomic.h \
  prov/sumi/include/rdma/fi_direct_cm.h \
  prov/sumi/include/rdma/fi_direct_eq.h \
  prov/sumi/include/rdma/fi_direct_trigger.h \
  prov/sumi/include/rdma/fi_direct.h

if SST_COMPILE_OSX
  common_sources += \
    src/unix/osd.c \
    include/osx/osd.h \
    include/unix/osd.h
else
  common_sources += \
    src/unix/osd.c \
    src/linux/osd.c \
    src/linux/rdpmc.c \
    include/linux/rdpmc.h \
    include/linux/osd.h \
    include/unix/osd.h
endif

compdir = $(pkglibdir)
comp_LTLIBRARIES = libfabric.la
libfabric_la_SOURCES = \
  $(sumi_sources) \
  $(sumi_headers) \
  $(sumi_rdma_headers) \
  $(common_sources) \
  $(libfabric_sources) \
  $(libfabric_headers) \
  $(rdmainclude_headers) \
  $(nodist_rdmainclude_headers)
