# Balar

SST GPGPU Simulation Components

## Installation

(Noted some of the components are pointed to personal repositories and will be moved once PRs are done)

### Prerequisites

Balar is tested with the following settings:

- gcc 7.5.0
- Ubuntu 18.04
- CUDA 10.1

The following components are needed to run Balar:

### [`sst-core`](https://github.com/sstsimulator/sst-core/tree/0f358dda178f96db3b0da88b2b965492c4be187d)

- Tested on commit `0f358dda178f96db3b0da88b2b965492c4be187d`
- Use `./configure --prefix=$SST_CORE_HOME --disable-mpi --disable-mem-pools` for sst-core config

### [`sst-elements`](https://github.com/William-An/sst-elements/tree/balar-mmio)

- Use `./configure --prefix=$SST_ELEMENTS_HOME --with-sst-core=$SST_CORE_HOME --with-cuda=$CUDA_INSTALL_PATH --with-gpgpusim=$GPGPUSIM_ROOT` for sst-elements config
- `$CUDA_INSTALL_PATH` should point to CUDA toolkit path
- `$GPGPUSIM_ROOT` will be set when sourcing the `setup_environment` script in `GPGPU-Sim`, which should point to its folder path

### [`GPGPU-Sim`](https://github.com/William-An/gpgpu-sim_distribution/tree/sst-integration)

```sh
# Pull GPGPU-Sim repo
git clone git@github.com:William-An/gpgpu-sim_distribution.git
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

### [`cudaAPITracer`](https://github.com/William-An/accel-sim-framework/tree/cuda_api_tracer)

We put the CUDA api tracer tool inside [Accel-Sim](https://github.com/William-An/accel-sim-framework/tree/cuda_api_tracer) framework in folder `ACCEL-SIM/util/tracer_nvbit/others/cuda_api_tracer_tool`, to install it:

```shell
# Get the Accel-Sim
git pull https://github.com/William-An/accel-sim-framework/tree/cuda_api_tracer

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

## Usage

After successful compilation and installation of SST core and SST elements (with GPGPUSim and CUDA), run:

```bash
# cd into balar
cd $SST_ELEMENTS_HOME/src/sst/balar

# balar tests
cd tests/

# Compile vector add sample CUDA program 
make -C vectorAdd

# Generate trace file for vectorAdd
LD_PRELOAD=PATH/TO/cuda_api_tracer_tool.so ./vectorAdd/vectorAdd

# Run with testCPU in tracer mode
# You will need the trace file for this, checkout `testBalar-testcpu.py` header
# for more information
sst testBalar-testcpu.py --model-options='-c ariel-gpu-v100.cfg -v -x vectorAdd/vectorAdd -t cuda_calls.trace'

# Compile vanadis binary and a custom CUDA API library
# (currently still under testing)
make -C vanadisHandshake/

# Run the handshake binary with vanadis core
sst testBalar-vanadis.py --model-options='-c ariel-gpu-v100.cfg'
```
