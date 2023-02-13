# Balar

SST GPGPU Simulation Components

## Usage

After successful compilation and installation of SST core and SST elements (with GPGPUSim and CUDA), run:

```bash
cd tests/

# Compile vector add sample CUDA program 
make -C vectorAdd

# Run with testCPU
# You will need the trace file for this, checkout `testBalar-testcpu.py` header
# for more information
sst testBalar-testcpu.py --model-options='-c ariel-gpu-v100.cfg -v -x vectorAdd/vectorAdd -t cuda_calls.trace'

# Compile vanadis binary and a custom CUDA API library
make -C vanadisHandshake/

# Run the handshake binary with vanadis core
sst testBalar-vanadis.py --model-options='-c ariel-gpu-v100.cfg'
```
