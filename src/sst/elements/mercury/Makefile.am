# -*- Makefile -*-
#
#

AM_CPPFLAGS += \
	$(MPI_CPPFLAGS) \
        -I$(top_srcdir)/src/sst/elements

# unpleasant hack to make vintage automake (e.g. 1.13.4) work
AM_LIBTOOLFLAGS = --tag=CXX

comp_LTLIBRARIES = libhg.la libsystemlibrary.la libcomputelibrary.la libostest.la
compdir = $(pkglibdir)

libhg_la_SOURCES = \
  common/event_link.cc \
  common/component.cc \
  common/hg_string.cc \
  common/loaderAPI.cc \
  common/mercury.cc \
  common/output.cc \
  common/util.cc \
  common/timestamp.cc \
  common/connection.cc \
  components/nic.cc \
  components/node.cc \
  components/node_base.cc \
  components/node_CL.cc \
  components/operating_system.cc \
  components/operating_system_base.cc \
  components/operating_system_CL.cc \
  hardware/common/packet.cc \
  hardware/common/recv_cq.cc \
  hardware/network/network_message.cc \
  libraries/compute/compute_scheduler.cc \
  libraries/compute/instruction_processor.cc \
  libraries/compute/memory_model.cc \
  operating_system/launch/app_launch_request.cc \
  operating_system/launch/app_launcher.cc \
  operating_system/libraries/library.cc \
  operating_system/libraries/event_library.cc \
  operating_system/libraries/unblock_event.cc \
  operating_system/process/app.cc \
  operating_system/process/global.cc \
  operating_system/process/progress_queue.cc \
  operating_system/process/thread.cc \
  operating_system/process/thread_info.cc \
  operating_system/threading/context_util.cc \
  operating_system/threading/stack_alloc.cc \
  operating_system/threading/stack_alloc_chunk.cc \
  operating_system/threading/thread_lock.cc \
  operating_system/threading/threading_fcontext.cc \
  pymercury.py
# not ready for this
#  threading/threading_pth.cc \
#  threading/threading_ucontext.cc

if SST_COMPILE_OSX
if SST_HG_X86
  libhg_la_SOURCES += \
    operating_system/threading/asm/make_x86_64_sysv_macho_gas.S \
    operating_system/threading/asm/jump_x86_64_sysv_macho_gas.S \
    operating_system/threading/asm/ontop_x86_64_sysv_macho_gas.S
endif
if SST_HG_A64
  libhg_la_SOURCES += \
    operating_system/threading/asm/make_arm64_aapcs_macho_gas.S \
    operating_system/threading/asm/jump_arm64_aapcs_macho_gas.S \
    operating_system/threading/asm/ontop_arm64_aapcs_macho_gas.S
endif
else
if SST_HG_X86
  libhg_la_SOURCES += \
    operating_system/threading/asm/make_x86_64_sysv_elf_gas.S \
    operating_system/threading/asm/jump_x86_64_sysv_elf_gas.S \
    operating_system/threading/asm/ontop_x86_64_sysv_elf_gas.S
endif
if SST_HG_A64
  libhg_la_SOURCES += \
    operating_system/threading/asm/make_arm64_aapcs_elf_gas.S \
    operating_system/threading/asm/jump_arm64_aapcs_elf_gas.S \
    operating_system/threading/asm/ontop_arm64_aapcs_elf_gas.S
endif
endif

libsystemlibrary_la_SOURCES = \
  libraries/system/system_library.cc

libcomputelibrary_la_SOURCES = \
  libraries/compute/compute_library.cc
#  libraries/compute/compute_scheduler.cc \
#  libraries/compute/instruction_processor.cc \
#  libraries/compute/memory_model.cc

libostest_la_SOURCES = \
  tests/ostest.cc

library_includedir=$(includedir)/sst/elements/mercury

nobase_library_include_HEADERS = \
  common/component.h \
  common/appLoader.h \
  common/loader.h \
  common/loaderAPI.h \
  common/connection.h \
  common/connection_fwd.h \
  common/event_link.h \
  common/events.h \
  common/hg_printf.h \
  common/errors.h \
  common/event_handler.h \
  common/node_address.h \
  common/stl_string.h \
  common/thread_lock.h \
  common/thread_safe_int.h \
  common/thread_safe_new.h \
  common/output.h \
  common/skeleton.h \
  common/skeleton_tls.h \
  common/printable.h \
  common/allocator.h \
  common/request_fwd.h \
  common/unique_id.h \
  common/util.h \
  common/request.h \
  common/null_buffer.h \
  common/sim_thread_lock.h \
  common/timestamp.h \
  common/unusedvariablemacro.h \
  components/nic.h \
  components/nic_fwd.h \
  components/node.h \
  components/node_fwd.h \
  components/node_base.h \
  components/node_base_fwd.h \
  components/node_CL.h \
  components/node_CL_fwd.h \
  components/operating_system_base.h \
  components/operating_system.h \
  components/operating_system_fwd.h \
  components/operating_system_CL.h \
  components/operating_system_CL_fwd.h \
  external/json.hpp \
  hardware/network/network_id.h \
  hardware/network/network_message.h \
  hardware/network/network_message_fwd.h \
  hardware/common/recv_cq.h \
  hardware/common/flow_fwd.h \
  hardware/common/packet.h \
  hardware/common/flow.h \
  hardware/common/packet_fwd.h \
  libraries/system/system_api.h \
  libraries/system/system_library.h \
  libraries/system/replacements/unistd.h \
  libraries/compute/compute_api.h \
  libraries/compute/compute_event.h \
  libraries/compute/compute_library.h \
  libraries/compute/compute_scheduler.h \
  libraries/compute/compute_scheduler_api.h \
  libraries/compute/instruction_processor.h \
  libraries/compute/memory_model.h \
  operating_system/launch/app_launcher.h \
  operating_system/launch/app_launcher_fwd.h \
  operating_system/launch/app_launch_request.h \
  operating_system/libraries/event_library_fwd.h \
  operating_system/libraries/service_fwd.h \
  operating_system/libraries/event_library.h \
  operating_system/libraries/service.h \
  operating_system/libraries/library.h \
  operating_system/libraries/library_fwd.h \
  operating_system/libraries/unblock_event.h \
  operating_system/threading/context_util.h \
  operating_system/threading/thread_lock.h \
  operating_system/threading/stack_alloc.h \
  operating_system/threading/stack_alloc_chunk.h \
  operating_system/threading/threading_interface.h \
  operating_system/threading/sim_thread_lock.h \
  operating_system/process/global.h \
  operating_system/process/cppglobal.h \
  operating_system/process/tls.h \
  operating_system/process/process_context.h \
  operating_system/process/software_id.h \
  operating_system/process/app.h \
  operating_system/process/thread_fwd.h \
  operating_system/process/thread_info.h \
  operating_system/process/thread.h \
  operating_system/process/memoize.h \
  operating_system/process/progress_queue.h \
  operating_system/process/thread_id.h \
  operating_system/process/app_id.h \
  operating_system/process/task_id.h \
  operating_system/process/mutex.h \
  operating_system/process/app_fwd.h \
  sst_hg_config.h

if !SST_ENABLE_PREVIEW_BUILD
libhg_la_SOURCES += $(deprecated_libhg_sources)
endif

EXTRA_DIST = \
    tests/testsuite_default_hg.py \
    tests/ostest.py \
    tests/refFiles/ostest.out

deprecated_EXTRA_DIST =

if !SST_ENABLE_PREVIEW_BUILD
EXTRA_DIST += $(deprecated_EXTRA_DIST)
endif


libhg_la_LDFLAGS = -module -avoid-version -shared
libsystemlibrary_la_LDFLAGS = -module -avoid-version -shared
libcomputelibrary_la_LDFLAGS = -module -avoid-version -shared
libostest_la_LDFLAGS = -module -avoid-version

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     mercury=$(abs_srcdir)
	$(SST_REGISTER_TOOL) SST_ELEMENT_TESTS      mercury=$(abs_srcdir)/tests

BUILT_SOURCES = \
   pymercury.inc

# This sed script converts 'od' output to a comma-separated list of byte-
# values, suitable for #include'ing into an array definition.
# This can be done much more simply with xxd or hexdump, but those tools
# are not installed by default on all supported platforms.
#
# od:   -v:             Print all data
#               -t x1:  Print as byte-values, in hex
# sed:  Script 1:  Remove base-address column from od output
#               Script 2:  Remove trailing blank line resulting from script 1
#               Script 3:  Add '0x' prefix, and ',' suffix to each value
%.inc: %.py
	od -v -t x1 < $< | sed -e 's/^[^ ]*[ ]*//g' -e '/^\s*$$/d' -e 's/\([0-9a-f]*\)[ $$]*/0x\1,/g' > $@

##########################################################################
##########################################################################
##########################################################################
