if SST_USE_ARIEL_MPI
CC=$(ARIEL_MPICC)
CXX=$(ARIEL_MPICXX)
# ffopenmp only needed to detect when MPI+OpenMP is used improperly
CFLAGS+=$(ARIEL_CFLAGS) -fopenmp
CXXFLAGS+=$(ARIEL_CXXFLAGS) -fopenmp
endif

AM_CPPFLAGS += -I../api $(MPI_CPPFLAGS) $(ARIEL_CFLAGS)
bin_PROGRAMS = hello reduce mpilauncher fakepin

mpilauncher_SOURCES = mpilauncher.cc
hello_SOURCES = hello.cc
reduce_SOURCES = reduce.cc
fakepin_SOURCES = fakepin.cc

hello_LDADD = ../api/libarielapi.la $(MPI_LIBS) $(MPILIBS)
reduce_LDADD = ../api/libarielapi.la $(MPI_LIBS) $(MPILIBS)

AM_DEFAULT_VERBOSITY = 1
