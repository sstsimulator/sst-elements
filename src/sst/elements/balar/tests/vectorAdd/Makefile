CXX=nvcc
CXXFLAGS=-m64 -gencode=arch=compute_60,code=sm_60 -gencode=arch=compute_61,code=sm_61 -gencode=arch=compute_70,code=sm_70 -gencode=arch=compute_70,code=compute_70 -ccbin gcc-5

vectorAdd: vectorAdd.o
	$(CXX) -O3 -Xcompiler -static-libgcc --cudart shared -o vectorAdd vectorAdd.o

vectorAdd.o: vecAdd.cu
	$(CXX) $(CXXFLAGS) -O3 -o vectorAdd.o -c vecAdd.cu

vectorAdd.objdump: vectorAdd
	objdump -Mno-aliases -Mnumeric --disassemble-all --disassemble-zeroes $< > $@

cuda_runtime_api.o: cuda_runtime_api.cc
	gcc -c -I./ -I$(CUDA_INSTALL_PATH)/include -Wall -Werror -fpic cuda_runtime_api.cc

libcudart.so.9.1: cuda_runtime_api.o
	gcc -shared -Wl,-soname,libcudart.so.9 -Wl,--version-script=linux-so-version.txt -o $@ $^
# ar rcs libcudart.so.9.1 static lib?

all: vectorAdd

clean:
	rm -f vectorAdd vectorAdd.o cuda_runtime_api.o libcudart.so.9.1


