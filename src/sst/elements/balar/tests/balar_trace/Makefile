CXX=nvcc
CXXFLAGS=-m64 -gencode=arch=compute_60,code=sm_60 -gencode=arch=compute_61,code=sm_61 -gencode=arch=compute_70,code=sm_70 -gencode=arch=compute_70,code=compute_70

vectorAdd: vectorAdd.o
	$(CXX) -O3 --cudart shared -o vectorAdd vectorAdd.o

vectorAdd.o: vecAdd.cu
	$(CXX) $(CXXFLAGS) -O3 -o vectorAdd.o -c vecAdd.cu

vectorAdd.objdump: vectorAdd
	objdump -Mno-aliases -Mnumeric --disassemble-all --disassemble-zeroes $< > $@

all: vectorAdd

clean:
	rm -f vectorAdd vectorAdd.o


