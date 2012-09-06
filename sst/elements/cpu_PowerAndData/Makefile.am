
if HAVE_MCPAT
if HAVE_PANALYZER

AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(MPI_CPPFLAGS) $(SIMPANALYZER_CPPFLAGS) $(MCPAT_CPPFLAGS)

compdir = $(pkglibdir)
comp_LTLIBRARIES = libcpu_PowerAndData.la
libcpu_PowerAndData_la_SOURCES = \
	cpu_PowerAndData.h \
	myMemEvent.h \
	cpu_PowerAndData.cc
libcpu_PowerAndData_la_LDFLAGS = -module -avoid-version $(SIMPANALYZER_LDFLAGS) $(MCPAT_LDFLAGS)
libcpu_PowerAndData_la_LIBADD = $(SIMPANALYZER_LIB) $(MCPAT_LIB)

endif
endif
