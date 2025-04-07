
bin_PROGRAMS = mpilauncher fakepin

mpilauncher_SOURCES = \
	$(srcdir)/mpilauncher.cc

fakepin_SOURCES = \
	$(srcdir)/fakepin.cc

mpilauncher_CXX = $(ARIEL_MPICXX)
fakepin_CXX = $(ARIEL_MPICXX)

mpilauncher_CXXFLAGS = $(AM_CFLAGS)
fakepin_CXXFLAGS = $(AM_CFLAGS)

all-local: copy-executables

copy-executables:
	@if test -f mpilauncher; then \
		echo "Copying mpilauncher to $(srcdir)"; \
		cp mpilauncher $(srcdir)/mpilauncher; \
	else \
		echo "mpilauncher not found; build may have failed."; \
	fi
	@if test -f fakepin; then \
		echo "Copying fakepin to $(srcdir)"; \
		cp fakepin $(srcdir)/fakepin; \
	else \
		echo "fakepin not found; build may have failed."; \
	fi

clean-local:
	rm -f mpilauncher fakepin
	rm -f $(srcdir)/mpilauncher $(srcdir)/fakepin


