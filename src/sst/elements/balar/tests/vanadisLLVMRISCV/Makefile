# Custom Vanadis CUDA lib
CUSTOM_CUDA_LIB_SRC := cuda_runtime_api_vanadis.cc
CUSTOM_CUDA_LIB := cudart_vanadis

# Clang path
CLANG_PREFIX := $(LLVM_INSTALL_PATH)/bin/

# RISCV Toolchain path
# Need to set RISCV_TOOLCHAIN_INSTALL_PATH env variable
RISCV_PREFIX := $(RISCV_TOOLCHAIN_INSTALL_PATH)/bin/riscv64-unknown-linux-gnu-
RISCV_SYSROOT := $(RISCV_TOOLCHAIN_INSTALL_PATH)/sysroot/
RISCV_CXX := $(wildcard $(RISCV_TOOLCHAIN_INSTALL_PATH)/riscv64-unknown-linux-gnu/include/c++/*/)

# Clang flags
CXX_FLAGS_RISCV_LINUX := --gcc-toolchain=$(RISCV_TOOLCHAIN_INSTALL_PATH) --sysroot=$(RISCV_SYSROOT) -I$(RISCV_CXX)/ -I$(RISCV_SYSROOT)/usr/include -I$(RISCV_CXX)/riscv64-unknown-linux-gnu --target=riscv64 -march=rv64gc
CXX_FLAGS_CUDA := --cuda-gpu-arch=$(GPU_ARCH) --cuda-path=$(CUDA_INSTALL_PATH) -L$(CUDA_INSTALL_PATH)/lib64 -lcudart -ldl -lrt -pthread
CXX_FLAGS_RISCV_CUDA := --cuda-gpu-arch=$(GPU_ARCH) --cuda-path=$(CUDA_INSTALL_PATH) -L. -Wl,-static -l$(CUSTOM_CUDA_LIB) -L$(CUDA_INSTALL_PATH)/lib64 -Wl,-static -ldl -lrt -pthread
CXX_FLAGS_CUDA_WRAPPER := -include __clang_cuda_runtime_wrapper.h

all: helloworld vecadd

test_llvm_env:
ifeq ($(LLVM_INSTALL_PATH),)
	$(error env CLANG_INSTALL_PATH is not set)
endif

test_vanadis_envs:
ifeq ($(RISCV_TOOLCHAIN_INSTALL_PATH),)
	$(error env RISCV_TOOLCHAIN_INSTALL_PATH is not set)
endif
ifeq ($(GPU_ARCH),)
	$(error env GPU_ARCH is not set)
endif

helloworld: helloworld.c test_llvm_env
	$(CLANG_PREFIX)clang -static -mno-relax $(CXX_FLAGS_RISCV_LINUX) $< -o $@

vanadis_cuda: $(CUSTOM_CUDA_LIB_SRC) test_vanadis_envs
	$(RISCV_PREFIX)gcc -c -static -I$(CUDA_INSTALL_PATH)/include -I. -DBALAR_CUDA_VERSION=\"libcudart_vanadis.a.$(shell echo $$CUDA_VERSION_NUMBER | cut -c 1-2)\" -fpic $<
	$(RISCV_PREFIX)gcc -shared -o lib$(CUSTOM_CUDA_LIB).so $(CUSTOM_CUDA_LIB_SRC:.cc=.o)
	$(RISCV_PREFIX)ar rcs lib$(CUSTOM_CUDA_LIB).a $(CUSTOM_CUDA_LIB_SRC:.cc=.o)

vecadd: vecadd.cu test_vanadis_envs test_llvm_env vanadis_cuda
	$(CLANG_PREFIX)clang++ $< -o $@ $(CXX_FLAGS_RISCV_CUDA) $(CXX_FLAGS_RISCV_LINUX) $(CXX_FLAGS_CUDA_WRAPPER)

vecadd_normal: vecadd.cu
	$(CLANG_PREFIX)clang++ $< -o $@ $(CXX_FLAGS_CUDA) $(CXX_FLAGS_CUDA_WRAPPER)

clean:
	rm -f lib$(CUSTOM_CUDA_LIB).so lib$(CUSTOM_CUDA_LIB).a $(CUSTOM_CUDA_LIB_SRC:.cc=.o) vecadd helloworld