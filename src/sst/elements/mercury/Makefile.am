# -*- Makefile -*-
#
#

AM_CPPFLAGS += \
	$(MPI_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libhg.la libsystemapi.la
libhg_la_SOURCES = \
  common/component.cc \
  common/hg_string.cc \
  common/mercury.cc \
  common/output.cc \
  common/util.cc \
  common/timestamp.cc \
  components/node.cc \
  components/operating_system.cc \
  hardware/common/packet.cc \
  hardware/common/recv_cq.cc \
  hardware/network/network_message.cc \
  operating_system/launch/app_launch_request.cc \
  operating_system/launch/app_launcher.cc \
  operating_system/libraries/api.cc \
  operating_system/libraries/library.cc \
  operating_system/libraries/unblock_event.cc \
  operating_system/process/app.cc \
  operating_system/process/simple_compute_scheduler.cc \
  operating_system/process/loadlib.cc \
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

libsystemapi_la_SOURCES = \
  libraries/system/system_api.cc

library_includedir=$(includedir)/sst/elements/hg

nobase_library_include_HEADERS = \
  common/thread_lock.h \
  common/events.h \
  common/hg_printf.h \
  common/errors.h \
  common/thread_safe_new.h \
  common/node_address.h \
  common/serializable.h \
  common/thread_safe_int.h \
  common/output.h \
  common/skeleton.h \
  common/printable.h \
  common/allocator.h \
  common/request_fwd.h \
  common/util.h \
  common/request.h \
  common/component.h \
  common/null_buffer.h \
  common/sim_thread_lock.h \
  common/timestamp.h \
  common/factory.h \
  components/operating_system_fwd.h \
  components/node.h \
  components/operating_system.h \
  components/node_fwd.h \
  external/json.hpp \
  hardware/network/network_id.h \
  hardware/network/network_message.h \
  hardware/network/network_message_fwd.h \
  hardware/common/recv_cq.h \
  hardware/common/flow_fwd.h \
  hardware/common/unique_id.h \
  hardware/common/packet.h \
  hardware/common/flow.h \
  hardware/common/packet_fwd.h \
  libraries/system/system_api.h \
  libraries/system/replacements/unistd.h \
  libraries/compute/lib_compute_time.h \
  libraries/compute/lib_compute_inst.h \
  libraries/compute/lib_compute.h \
  libraries/compute/compute_api.h \
  libraries/compute/lib_compute_fwd.h \
  libraries/compute/lib_hybrid_compute.h \
  libraries/compute/compute_event.h \
  libraries/compute/lib_compute_memmove.h \
  libraries/compute/compute_event_fwd.h \
  operating_system/launch/app_launcher.h \
  operating_system/launch/app_launcher_fwd.h \
  operating_system/launch/app_launch_request.h \
  operating_system/libraries/library_fwd.h \
  operating_system/libraries/service_fwd.h \
  operating_system/libraries/library.h \
  operating_system/libraries/service.h \
  operating_system/libraries/api.h \
  operating_system/libraries/api_fwd.h \
  operating_system/libraries/unblock_event.h \
  operating_system/threading/context_util.h \
  operating_system/threading/thread_lock.h \
  operating_system/threading/stack_alloc.h \
  operating_system/threading/stack_alloc_chunk.h \
  operating_system/threading/threading_interface.h \
  operating_system/threading/sim_thread_lock.h \
  operating_system/process/tls.h \
  operating_system/process/process_context.h \
  operating_system/process/software_id.h \
  operating_system/process/app.h \
  operating_system/process/compute_scheduler_fwd.h \
  operating_system/process/thread_fwd.h \
  operating_system/process/thread_info.h \
  operating_system/process/compute_scheduler.h \
  operating_system/process/thread.h \
  operating_system/process/simple_compute_scheduler.h \
  operating_system/process/loadlib.h \
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
    tests/node.py

deprecated_EXTRA_DIST =

if !SST_ENABLE_PREVIEW_BUILD
EXTRA_DIST += $(deprecated_EXTRA_DIST)
endif


libhg_la_LDFLAGS = -module -avoid-version
libsystemapi_la_LDFLAGS = -module -avoid-version

install-exec-hook:
	$(SST_REGISTER_TOOL) SST_ELEMENT_SOURCE     mercury=$(abs_srcdir)

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
