CXXFLAGS+=-fopenmp
mpilauncher_CXX = $(ARIEL_MPICXX)
reduce_CXX = $(ARIEL_MPICXX)
hello_CXX = $(ARIEL_MPICXX)
fakepin_CXX = $(ARIEL_MPICXX)

AM_CPPFLAGS += -I../api $(ARIEL_MPI_CFLAGS)
bin_PROGRAMS = hello reduce mpilauncher fakepin

hello_LDADD = -L../api -larielapi $(ARIEL_MPI_LIBS)
reduce_LDADD = -L../api -larielapi $(ARIEL_MPI_LIBS)
#reduce_LDADD = -L../api -larielapi $(ARIEL_MPI_LIBS)

mpilauncher_SOURCES = mpilauncher.cc
hello_SOURCES = hello.cc
reduce_SOURCES = reduce.cc
fakepin_SOURCES = fakepin.cc

