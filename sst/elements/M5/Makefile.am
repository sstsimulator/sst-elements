# -*- Makefile -*-
#
#

EXTRA_DIST = m5.xml

compdir = $(pkglibdir)
comp_LTLIBRARIES = libm5C.la


libm5C_la_SOURCES = \
    barrier.h \
    baseCache.cpp \
    bounce.h \
    bridge.cpp \
    bridge.h \
    bus.cpp \
    busPlus.cpp \
    busPlus.h \
    debug.cpp \
    trace.cpp\
    trace.h\
    dummyPhysicalMemory.h \
    dummySystem.h \
    dummyThreadContext.h \
    exeBackingMem.h \
    factory.h \
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
    power.cpp \
    ptlNicMMIF.cpp \
    ptlNicMMIF.h \
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

if MODEL_POWER
libm5C_la_SOURCES += ../power/power.cc \
		../power/power.h \
		../power/interface.h \
		../power/HotSpot-interface.cc \
		../power/HotSpot-interface.h \
		../power/reliability.cc \
		../power/reliability.h
endif

if HAVE_DRAMSIM 
libm5C_la_SOURCES += DRAMSimWrap.cpp DRAMSimWrap.h \
            DRAMSimWrap2.cpp DRAMSimWrap2.h \
            DRAMSimWrapPlus.cpp DRAMSimWrapPlus.h \
            DRAMSimWrap2Plus.cpp DRAMSimWrap2Plus.h
endif

libm5C_la_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS) $(M5_CPPFLAGS) \
            $(DRAMSIM_CPPFLAGS) \
            -I$(top_srcdir)/sst/elements/include \
            -I$(top_srcdir)/sst/elements


libm5C_la_LDFLAGS = -module -avoid-version $(M5_LDFLAGS) $(DRAMSIM_LDFLAGS)
libm5C_la_LIBADD = $(M5_LIB) $(DRAMSIM_LIB)

if MODEL_POWER
libm5C_la_CPPFLAGS += $(MCPAT_CPPFLAGS) $(HOTSPOT_CPPFLAGS) \
		      -DM5_WITH_POWER 
libm5C_la_LDFLAGS += $(MCPAT_LDFLAGS) $(HOTSPOT_LDFLAGS) 
libm5C_la_LIBADD += $(MCPAT_LIB) $(HOTSPOT_LIB) 
endif

