bin_PROGRAMS = md md_pim

CPPFILES = \
		   atom.cpp \
		   comm.cpp \
		   force.cpp \
		   input.cpp \
		   integrate.cpp \
		   ljs.cpp \
		   neighbor.cpp \
		   output.cpp \
		   setup.cpp \
		   thermo.cpp \
		   timer.cpp

md_SOURCES = $(CPPFILES)

md_pim_SOURCES = $(CPPFILES)
md_pim_CPPFLAGS = -DPIM_EXTENSIONS

noinst_HEADERS = \
			 atom.h \
			 comm.h \
			 force.h \
			 integrate.h \
			 ljs.h \
			 neighbor.h \
			 thermo.h \
			 timer.h
EXTRA_DIST = lj.in
