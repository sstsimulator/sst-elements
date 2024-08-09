all-local: mpilauncher

mpilauncher:
	 $(ARIEL_MPICXX) mpilauncher.cc -o mpilauncher
fakepin:
	 $(ARIEL_MPICXX) fakepin.cc -o fakepin
hello:
	 $(ARIEL_MPICXX) hello.cc -o hello -I../api -L../api -larielapi -fopenmp
reduce:
	 $(ARIEL_MPICXX) reduce.cc -o reduce -I../api -L../api -larielapi -fopenmp

clean-local:
	rm -f mpilauncher fakepin
