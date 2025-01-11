# Balar

SST GPGPU-Sim integration component.
Support running CUDA programs via trace-driven (require GPU for trace generation) and direct-execution (CUDA program runs with Vanadis RISC-V CPU component).

## Installation

Balar tested with the following settings:

- gcc 11.4.0
- Ubuntu 22.04.4 LTS
- CUDA 11.7

The following components are needed to run Balar:

### General

#### [`sst-core`](https://github.com/sstsimulator/sst-core)

- Use `./configure --prefix=$SST_CORE_HOME` for sst-core config

#### [`sst-elements`](https://github.com/sstsimulator/sst-elements)

- Use `./configure --prefix=$SST_ELEMENTS_HOME --with-sst-core=$SST_CORE_HOME --with-cuda=$CUDA_INSTALL_PATH --with-gpgpusim=$GPGPUSIM_ROOT` for sst-elements config
- `$CUDA_INSTALL_PATH` should point to CUDA toolkit path
- `$GPGPUSIM_ROOT` will be set when sourcing the `setup_environment` script in `GPGPU-Sim`, which should point to its folder path

#### [`GPGPU-Sim`](https://github.com/accel-sim/gpgpu-sim_distribution)

```sh
# Pull GPGPU-Sim repo
git clone git@github.com:accel-sim/gpgpu-sim_distribution.git
cd gpgpu-sim_distribution

# Make sure $CUDA_INSTALL_PATH is set
echo $CUDA_INSTALL_PATH

# Setup GPGPU-Sim in SST-integration mode
source setup_environment sst

# Build GPGPU-Sim
make -j
```

> Noted GPGPU-Sim has some prerequisites:
> ```shell
> # GPGPU-Sim dependencies
> sudo apt-get install build-essential xutils-dev bison zlib1g-dev flex libglu1-mesa-dev```

### Trace-driven

#### [`cudaAPITracer`](https://github.com/accel-sim/accel-sim-framework)

We put the CUDA api tracer tool inside [Accel-Sim](https://github.com/accel-sim/accel-sim-framework) framework in folder `ACCEL-SIM/util/tracer_nvbit/others/cuda_api_tracer_tool`, to install it:

```shell
# Get the Accel-Sim framework
git clone git@github.com:accel-sim/accel-sim-framework.git

# cd into tracer tool folder
cd accel-sim-framework/util/tracer_nvbit

# Install nvbit
./install_nvbit.sh

# Compile tracer tool
# Which will generate a 'cuda_api_tracer_tool.so' file at
# './others/cuda_api_tracer_tool/cuda_api_tracer'
make -C ./others/cuda_api_tracer_tool
```

To run the tracer tool on a CUDA binary:
```shell
LD_PRELOAD=PATH/TO/cuda_api_tracer_tool.so CUDA_EXE
```

Which will generate the following files when exiting:

- `cuda_calls.trace`: the API trace file tracking
    - `cudaMemcpy`
    - `cudaMalloc`
    - cuda kernel launches
    - `cudaFree`
- `cuMemcpyD2H-X-X.data`: cuda memcpy device to host data payload
- `cuMemcpyH2D-X-X.data`: cuda memcpy host to device data payload

> Noted the API tracer need a machine with GPU

### Direct-execution

In order to run CUDA program directly, the CUDA source code needs to be recompiled with LLVM and RISC-V GCC toolchain.

#### LLVM and RISCV GCC toolchain

```bash
## Build LLVM with RISC-V, x86, and CUDA support from source
git clone https://github.com/llvm/llvm-project.git

mkdir llvm-install
cd llvm-project
mkdir build && cd build
cmake -DLLVM_TARGETS_TO_BUILD="RISCV;X86;NVPTX" -DLLVM_DEFAULT_TARGET_TRIPLE=riscv64-unknown-linux-gnu \
      -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_PROJECTS="clang;lld" -DCMAKE_INSTALL_PREFIX=$LLVM_INSTALL_PATH ../llvm
cmake --build . -j30
cmake --build . --target install
cd ..

## Build RISC-V GCC toolchain
git clone https://github.com/riscv-collab/riscv-gnu-toolchain.git

mkdir riscv-gnu-install
cd riscv-gnu-toolchain
./configure --prefix=$RISCV_INSTALL_PATH
make linux -j
cd ..

# Set up environment vars to LLVM and RISCV GCC
export LLVM_INSTALL_PATH=$(pwd)/llvm-install
export RISCV_TOOLCHAIN_INSTALL_PATH=$(pwd)/riscv-gnu-install
# Match with the GPU config file we have
export GPU_ARCH=sm_70
```

#### Compiling CUDA for Balar + Vanadis

In order to run CUDA on Balar + Vanadis (direct-execution), aside from the compilers, the custom CUDA library `libcudart_vanadis` (inside `vanadisLLVMRISCV`) is also needed to intercept CUDA API calls and send them to Balar's MMIO address. This custom CUDA lib can be made via `make -C vanadisLLVMRISCV vanadis_cuda`, which would generate the `libcudart_vanadis.a/so`.

For compiler and linker flags, you can refer to the `vecadd` target in `vanadisLLVMRISCV/Makefile`.

#### GPU Application Collection

We are working on getting a collection of GPU apps to run with Balar+Vanadis. Those benchmarks are from the [gpu-app-collection](https://github.com/accel-sim/gpu-app-collection) repo.

```bash
git clone git@github.com:accel-sim/gpu-app-collection.git
cd gpu-app-collection
git checkout sst_support

# Setup environ vars for apps, need to have
# env var LLVM_INSTALL_PATH and RISCV_TOOLCHAIN_INSTALL_PATH
# If you plan to compile the apps directly, you will 
# also need to set SST_CUSTOM_CUDA_LIB_PATH to 
# the directory of the custom CUDA library,
# which normally will be `SST-ELEMENT-SOURCE/src/sst/elements/balar/tests/vanadisLLVMRISCV`
source ./src/setup_environment sst
```

## Usage

### Trace-driven Mode

After successful compilation and installation of SST core and SST elements (with GPGPUSim and CUDA), run:

```bash
# cd into balar
cd SST_ELEMENTS_SRC/src/sst/elements/balar

# balar tests
cd tests/

# Compile vector add sample CUDA program 
make -C vectorAdd

# Generate trace file for vectorAdd
LD_PRELOAD=PATH/TO/cuda_api_tracer_tool.so ./vectorAdd/vectorAdd

# Run with testCPU in tracer mode
# You will need the trace file for this, checkout `testBalar-testcpu.py` header
# for more information
sst testBalar-testcpu.py --model-options='-c gpu-v100-mem.cfg -v -x vectorAdd/vectorAdd -t cuda_calls.trace'

# Compile vanadis binary and a custom CUDA API library
# (currently still under testing)
make -C vanadisHandshake/

# Run the handshake binary with vanadis core
sst testBalar-vanadis.py --model-options='-c gpu-v100-mem.cfg'
```

### Vanadis Mode

The CUDA executable should be passed in `VANADIS_EXE` and `BALAR_CUDA_EXE_PATH`. If there are args to the program, they should be passed with `VANADIS_EXE_ARGS`.

```bash
# cd into balar tests
cd SST_ELEMENTS_SRC/src/sst/elements/balar/tests/

# Compile test programs
make -C vanadisLLVMRISCV

# Run CPU only program
VANADIS_EXE=./vanadisLLVMRISCV/helloworld VANADIS_ISA=RISCV64 sst testBalar-vanadis.py --model-options='-c gpu-v100-mem.cfg'

# Run sample vecadd
VANADIS_EXE=./vanadisLLVMRISCV/vecadd VANADIS_ISA=RISCV64 BALAR_CUDA_EXE_PATH=./vanadisLLVMRISCV/vecadd sst testBalar-vanadis.py --model-options='-c gpu-v100-mem.cfg'
```

### Running GPU Benchmark

Here is an example on running Rodinia 2.0 BFS with SampleGraph.txt input using CUDA 11.7. For different CUDA version, the binary path will differ in terms of version number.

```bash
# Let GPU app knows about the custom CUDA lib
export SST_CUSTOM_CUDA_LIB_PATH=SST_ELEMENTS_SRC/src/sst/elements/balar/tests/vanadisLLVMRISCV

# Make Rodinia 2.0
cd gpu-app-collection
make rodinia_2.0-ft -i -j -C ./src
make data -C ./src
cd ..

# Run BFS with sample graph input
cd SST_ELEMENTS_SRC/src/sst/elements/balar/tests
VANADIS_EXE=$GPUAPPS_ROOT/bin/11.7/release/bfs-rodinia-2.0-ft \
VANADIS_EXE_ARGS=$GPUAPPS_ROOT/data_dirs/cuda/rodinia/2.0-ft/bfs-rodinia-2.0-ft/data/SampleGraph.txt \
VANADIS_ISA=RISCV64 \
BALAR_CUDA_EXE_PATH=$GPUAPPS_ROOT/bin/11.7/release/bfs-rodinia-2.0-ft sst \
testBalar-vanadis.py --model-options='-c gpu-v100-mem.cfg'
```

### Running Unittest

Balar's unittest suites will automatically compile the GPU app collection with the LLVM and RISCV toolchain and run them.

```bash
sst-test-elements -w "*balar*"
```