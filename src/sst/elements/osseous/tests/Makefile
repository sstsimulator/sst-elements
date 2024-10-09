CXX=g++
APIDIR=./
INCLUDE=./../eras_datatypes/

testbench: testbench.o
	$(CXX) -O3 -o testbench testbench.o -I$(INCLUDE) -L$(APIDIR) -Wl,-rpath=$(APIDIR) -ltb_header 

testbench.o: testbench.c
	$(CXX) -O3 -I$(APIDIR) -I$(INCLUDE) -o testbench.o -c testbench.c

tb_header.o: tb_header.c tb_header.h
	$(CXX) -c -O3 -o tb_header.o -I./ -I$(INCLUDE) -fPIC tb_header.c

libtb_header.so: tb_header.o
	$(CXX) -shared -O3 -fPIC -o libtb_header.so tb_header.o

libtb_header.a: tb_header.o
	ar rcs $@ $^

all: libtb_header.so libtb_header.a testbench 

clean:
	rm testbench testbench.o libtb_header.so tb_header.o libtb_header.a

