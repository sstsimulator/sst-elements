#### Osseous

Osseous is a generic RTL Component template, a major portion of ERAS framework, which is updated based on the user requirement to generate dynamically loadable component with SST. The automation flow of the ERAS framework set up by the parser takes in the generic RTL template and C-model header as input and emits final dynamically loadable SST component library integrable with SST.

The repository contain _osseous_ and an example of dynamically loadable component formed out of osseous. 
Currently, the example is a simple vector shift register(_vecshiftreg_).

```
osseous
osseous/examples/vecshiftreg
```
To build and register _osseous_ & _vecshiftreg_ and run the shift register example, below are the steps.
```
clone the repository containing osseous

Update other Env variables as part of standard sst-elements build process(SST_ELEMENTS_HOME, SST_CORE_HOME)
cd path/to/sst-elements/
./autogen.sh
./configure --prefix=$SST_ELEMENTS_HOME --with-sst-core=$SST_CORE_HOME --with-pin=$PINHOME
make all
make install
export PATH=$SST_CORE_HOME/bin:$SST_ELEMENTS_HOME/bin:$PATH

cd osseous/examples/vecshiftreg/tests
make all (compiles RTL testbench into executable that is fed to Ariel)
sst vecshiftreg.py
```
Note that Ariel and its links with vecshiftreg has to be specified in python configuration file for appropriate functionality. The statistics collected will be visible at the end of simulation. The cycle count of RTL testbench has to be increased by significant figure to observe notable changes in simulation time due to RTL simulation.

**Modifying C-testbench: **
```
osseous/examples/vecshiftreg/tests/testbench.c
```
Provided testbench (_testbench.c_) contains both non-RTL computations (weighted matrix addition) and RTL testvectors. 
It has following APIs:

**User APIs:**
```
start_RTL_sim(shmem) - To begin RTL simulation and pass the virtual addresses of RTL inputs(shmem) to send it to DUT. Ariel sends an event to DUT (RTL Component) to initiate simulation
update_RTL_sig(shmem) - To update the RTL input signals while simulation progresses.
```
**Supporting APIs (Definition in tb_header.c):**
```
RTL_shmem_info(): mem malloc space to store RTL inputs into memH (intercepted by PIN)
Update_RTL_params(): Updates RTL params to defaults

perform_update(upd_inp, upd_ctrl, upd_eval_args, upd_reg, verbose, done_rst, sim_done, sim_cycles): RTL parameters specified as arguments
bool upd_inp: Update RTL inputs?
bool upd_ctrl: Update control data?
bool upd_eval_args: Update eval() API arguments? eval() is primary API of C-model header

storetomem(): Updates the RTL parameters in memH (stores are intercepted by PIN)
```
**_eval(upd_reg, verbose, done_rst)_ API Arguments (Part of C-model header):**
```
bool upd_reg: Update C-model registers?
bool verbose: Print debug information?
bool done_rst: NA (Not used)
bool sim_done: simulation done? ("true" marks end of simulation after specified cycles)
uint64_t sim_cyles: specifies number of SST cycles RTL C-model should simulate with provided inputs (shmem)
```
Eval API arguments are passed along with rest of the RTL inputs to be stored into memH.

**Statistics collected:**
```
 vecshiftregister. : Accumulator : Sum.u64 = 9; SumSQ.u64 = 9; Count.u64 = 9; Min.u64 = 1; Max.u64 = 1; (read_requests)
 vecshiftregister. : Accumulator : Sum.u64 = 0; SumSQ.u64 = 0; Count.u64 = 0; Min.u64 = 0; Max.u64 = 0; (write_requests)
 vecshiftregister. : Accumulator : Sum.u64 = 0; SumSQ.u64 = 0; Count.u64 = 0; Min.u64 = 0; Max.u64 = 0; (write_request_sizes)
 vecshiftregister. : Accumulator : Sum.u64 = 0; SumSQ.u64 = 0; Count.u64 = 0; Min.u64 = 0; Max.u64 = 0; (split_read_requests)
 vecshiftregister. : Accumulator : Sum.u64 = 0; SumSQ.u64 = 0; Count.u64 = 0; Min.u64 = 0; Max.u64 = 0; (split_write_requests)
 ```
