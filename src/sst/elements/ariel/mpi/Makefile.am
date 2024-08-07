all-local: mpilauncher fakepin hello reduce

mpilauncher:
	 mpicxx mpilauncher.cc -o mpilauncher
fakepin:
	 mpicxx fakepin.cc -o fakepin
hello:
	 mpicxx hello.cc -o hello -I../api -L../api -larielapi -fopenmp
reduce:
	 mpicxx reduce.cc -o reduce -I../api -L../api -larielapi -fopenmp
