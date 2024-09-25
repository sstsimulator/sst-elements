all: hello reduce

clean:
	rm -f hello reduce

API_DIR=../../api

hello:
	     mpicxx hello.cc -o hello -I$(API_DIR) -L$(API_DIR) -larielapi -fopenmp
reduce:
	     mpicxx reduce.cc -o reduce -I$(API_DIR) -L$(API_DIR) -larielapi -fopenmp
