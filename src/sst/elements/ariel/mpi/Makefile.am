
bin_PROGRAMS = mpilauncher fakepin

mpilauncher_SOURCES = \
	$(srcdir)/mpilauncher.cc

fakepin_SOURCES = \
	$(srcdir)/fakepin.cc

mpilauncher_CXX = $(ARIEL_MPICXX)
fakepin_CXX = $(ARIEL_MPICXX)

mpilauncher_CXXFLAGS = $(AM_CFLAGS)
fakepin_CXXFLAGS = $(AM_CFLAGS)

clean-local:
	rm -f mpilauncher fakepin


