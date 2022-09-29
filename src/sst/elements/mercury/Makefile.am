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

libsystemapi_la_SOURCES = \
  libraries/system/system_api.cc

library_includedir=$(includedir)/sst/elements/hg

nobase_library_include_HEADERS = \
  common/null_buffer.h \
  common/skeleton.h \
  components/operating_system.h \
  libraries/system/system_api.h \
  operating_system/process/thread.h \
  operating_system/libraries/api.h \
  operating_system/libraries/api_fwd.h
#  
#  process/app.h \
#  process/app_id.h \
#  process/mutex.h \
#  process/process_context.h \
#  process/software_id.h \
#  process/task_id.h \
#  process/thread.h \
#  process/thread_id.h \
#  process/thread_info.h \
#  process/tls.h \
#  threading/context_util.h \
#  threading/sim_thread_lock.h \
#  threading/stack_alloc.h \
#  threading/stack_alloc_chunk.h \
#  threading/thread_lock.h \
#  threading/threading_interface.h

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
