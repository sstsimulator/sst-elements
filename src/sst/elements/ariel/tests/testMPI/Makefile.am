all-local: hello reduce

clean-local:
	rm -f hello reduce

API_DIR=../../api

hello:
	     $(ARIEL_MPICXX) hello.cc -o hello -I$(API_DIR) -L$(API_DIR) -larielapi -fopenmp
reduce:
	     $(ARIEL_MPICXX) reduce.cc -o reduce -I$(API_DIR) -L$(API_DIR) -larielapi -fopenmp
