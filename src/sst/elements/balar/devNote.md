# Balar

## TODO

1. [x] Link the custom cuda lib with nvcc executable
2. [x] Create an interface in Balar using MMIO to accept arguments and send results
    1. [x] Test if can receive and send data properly with standardCPU
    1. Run script: `sst testBalar-simple.py --model-options="-c ariel-gpu-v100.cfg -v" > tmp.out 2>&1`
3. [ ] Like a unit-test feel use StandardCPU to pass stimulus (CUDA arguments) to balar
4. [ ] Pass PTX file directly? 