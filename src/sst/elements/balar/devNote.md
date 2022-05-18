# Balar

## TODO

1. [x] Link the custom cuda lib with nvcc executable
2. [x] Create an interface in Balar using MMIO to accept arguments and send results
    1. [x] Test if can receive and send data properly with standardCPU
    1. Need to source the gpgpusim and cuda script
        1. `source ../env-setup/sst_env_setup.sh `
        1. `source ../sst-gpgpusim-external/setup_environment`
    1. Need to make and install the sst
        1. `make -j`
        1. `make install`
    1. Run script: `sst testBalar-simple.py --model-options="-c ariel-gpu-v100.cfg -v" > tmp.out 2>&1`
3. [ ] Real test
    1. [x] How to generate test stimulus?
    2. [x] Actually pass arguments to gpgpusim
    3. [x] VectorAdd example?
    4. [x] Get cuda calls arguments from old balar
        1. [ ] Automated dump cuda call sequences and generate launch/config file to the testcpu to consume
        2. [x] Or NVBit CUDA calls?
4. [ ] Like a unit-test feel use StandardCPU to pass stimulus (CUDA arguments) to balar
5. [ ] Pass PTX file directly?
6. [ ] What about timing simulation for cuda calls? Earlier one has gpu caches and all sort of thing
7. [ ] SST Cuda call callbacks in `balarMMIO.cc` to simulate cache latency in SST?

## Issues

1. [x] Need to wait for kernel launch to be completed before issuing D2H copies?
    1. [ ] Need to understand the stream operation
    1. [ ] Never execute `stream_operation::do_operation` when a kernel exists ahead, should be blocking though?
    1. [ ] Probable cause in `void stream_manager::push( stream_operation op )` not waiting for stream 0 to be empty? Thus D2H copy is skipped
        1. Disable it will not change the result
    1. [ ] Right now the design is to fit a call in one memory handle
        1. However, we need to delay the response send to testcpu
        2. Send the response when the api call finishes in `tick` function
2. [ ] Simulation stuck at `add.s32 %r8, %r6, %r7;` for vectorAdd
    1. Looks like it is waiting for the register to be loaded from global memory
    1. `ld.global.u32 %r6, [%rd8];` and `ld.global.u32 %r7, [%rd6];`
    1. Issue with memory unit loading? But params loading works?
        1. Maybe memcpyH2D is wrong? Use the SST version?
        2. Or need other config on memory subsystem?
    1. Config for SST GPGPUSIM is different
        1. Mem subsystem is turned off
    1. Didnot implement `SST_pop_mem_reply`, which is used to push mem requests from SST cache to GPGPUSim
        1. `shader.cc:4121`
        1. Need to construct the memory subsystem for balarMMIO then
        1. Check `icnt_inject_request_packet_to_SST()`
            1. Send read/write request from GPU to SST
            1. SST(balar) handles the cache request and timing
            1. Then it send back data to GPU using the `SST_receive_mem_reply()`, which push the memory packet to icnt for the GPU to access
                1. i.e. the load and store data goes here

## Caveats

1. Cuda memcpy data pointer need to remain valid under the operation finish
