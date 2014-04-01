# -*- Makefile -*-
#
#

compdir = $(pkglibdir)
comp_LTLIBRARIES = libm5C.la

libm5C_la_SOURCES = \
    barrier.h \
    baseCache.cpp \
    bounce.h \
    bridge.cpp \
    bus.cpp \
    busPlus.cpp \
    busPlus.h \
    debug.cpp \
    trace.cpp\
    trace.h\
    dummySystem.h \
    dummyThreadContext.h \
    factory.h \
    factory.cpp \
    loadMemory.cpp \
    loadMemory.h \
    m5.cpp \
    m5.h \
    m5lib.cpp \
    memEvent.h \
    memLink.cpp \
    memLink.h \
    physicalPlus.cpp \
    physicalPlus.h \
    physicalMemory.cpp \
	portLink.h \
	portLink.cpp \
    power.cpp \
    process.h \
    simpleCpu.cpp \
    syscall.cpp \
    syscall.h \
    system.h \
    util.cpp \
    util.h

if USE_M5_O3
libm5C_la_SOURCES += O3Cpu.cpp
endif

if ENABLE_GEM5_POWER_MODEL
if USE_MCPAT
if USE_HOTSPOT
libm5C_la_SOURCES += power_power.cc \
		power_HotSpot-interface.cc \
		power_reliability.cc
endif
endif
endif

# Work around bad habit of including source files from another component.
power_power.cc: $(top_srcdir)/sst/elements/power/power.cc
	@cp $(top_srcdir)/sst/elements/power/power.cc $(srcdir)/power_power.cc
power_HotSpot-interface.cc: $(top_srcdir)/sst/elements/power/HotSpot-interface.cc
	@cp $(top_srcdir)/sst/elements/power/HotSpot-interface.cc $(srcdir)/power_HotSpot-interface.cc
power_reliability.cc: $(top_srcdir)/sst/elements/power/reliability.cc
	@cp $(top_srcdir)/sst/elements/power/reliability.cc $(srcdir)/power_reliability.cc

if HAVE_DRAMSIM 
libm5C_la_SOURCES += DRAMSimWrap.cpp DRAMSimWrap.h \
            DRAMSimWrap2.cpp DRAMSimWrap2.h \
            DRAMSimWrapPlus.cpp DRAMSimWrapPlus.h \
            DRAMSimWrap2Plus.cpp DRAMSimWrap2Plus.h
endif

if HAVE_PHXSIM 
libm5C_la_SOURCES += PHXSimWrap.cpp PHXSimWrap.h
endif

libm5C_la_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS) $(M5_CPPFLAGS) \
            $(DRAMSIM_CPPFLAGS) \
            $(PHXSIM_CPPFLAGS) \
	    $(PYTHON_CPPFLAGS) \
            -I$(top_srcdir)/sst/elements/include \
            -I$(top_srcdir)/sst/elements

libm5C_la_LDFLAGS = -module -avoid-version $(M5_LDFLAGS) $(DRAMSIM_LDFLAGS) $(PHXSIM_LDFLAGS)
libm5C_la_LIBADD = $(M5_LIB) $(DRAMSIM_LIB) $(PHXSIM_LIB)

if ENABLE_GEM5_POWER_MODEL
if USE_MCPAT
if USE_HOTSPOT
libm5C_la_CPPFLAGS += $(MCPAT_CPPFLAGS) $(HOTSPOT_CPPFLAGS) -I$(top_srcdir)/sst/elements/power \
		      -DM5_WITH_POWER 
libm5C_la_LDFLAGS += $(MCPAT_LDFLAGS) $(HOTSPOT_LDFLAGS) 
libm5C_la_LIBADD += $(MCPAT_LIB) $(HOTSPOT_LIB)
endif
endif
endif
