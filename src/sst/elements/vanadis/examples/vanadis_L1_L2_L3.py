#
# This example demonstrates a how to model a single node and CPU on a RISC-V architecture in vanadis.
# The node modeled has L1D, L1I, L2, and L3 caches. 
# 
#  In order to accomplish this structure in SST, we create the following comoponenents and links.
#
# _______________________________________________________           ______________________________________________________________________________________________________
# | Component: OS                                       |           | Component: CPU                                                                                     |                                     
# | Type: vanadis.VanadisNodeOs                         |           | Type: vanadis.dbg_vanadisCPU                                                                       |
# | This represents the hardware node being modeled.    |           | This component represents the CPU being modeled. It has links ot the OS component to represent     |
# | It contains an mmu (memory management unit, which   |           | the operating system running on the node, as well as links to the L1D and L1I caches for the node. |
# | also provides TLB capabilities) and  a memory       |           |                                                                                                    |
# | interface.                                          |----Link---|  ________________________________________________                                                  |
# |  _______________________                            |           |  | Sub Component: decoder0                      |                                                  | 
# |  | Sub Component: mmu  |                            |           |  | Type: vanadis.VanadisRISCV64Decoder          |                                                  |
# |  | Type: mmu.simpleMMU |                            |           |  | This subcomponent represents the instruction |                                                  |
# |  |_____________________|                            |           |  | decoder of the CPU                           |                                                  |
# |  ____________________________________________       |           |  |  _________________________________________   |                                                  |
# |  | Sub Component: mem_interface             |       |           |  |  | Sub Component: os_handler             |   |                                                  |
# |  | Type: memHierarchy.standardInterface     |       |           |  |  | Type:vanadis.VanadisRISCV64DOSHandler |   |                                                  | 
# |  | This subcomponent represents the         |       |           |  |  |_______________________________________|   |                                                  |
# |  | interface from the Node OS to the cache. |       |           |  |  _________________________________________   |                                                  |
# |  |__________________________________________|       |           |  |  | Sub Component: branch_unit            |   |                                                  |
# |_________________________|___________________________|           |  |  | Type: vanadis.VanadisBasicBranchUnit  |   |                                                  |
#                           | Link                                  |  |  |_______________________________________|   |                                                  |
#         __________________|___________________                    |  |______________________________________________|                                                  |
#         | Component: node_os.cache           |                    |  ________________________________________________                                                  |
#         | Type: memHierarchy.Cache           |                    |  | Sub Component: lsq                           |                                                  |
#         | This componenet represents the L1D |                    |  | Type: vanadis.VanadisBasicLoadStoreQueue     |                                                  |
#         | cache for the hardware node.       |                    |  |  __________________________________________  |    __________________________________________    |
#         |____________________________________|                    |  |  | Sub Component: memory_interface        |  |    | Sub Component: mem_interface_inst      |    |                                              
#                            |                                      |  |  | Type: memHierarchy.standardInterface   |  |    | Type: memHierarchy.standardInterface   |    |                                              
#                            |                                      |  |  | This subcomponent represents the CPU's |  |    | This subcomponent represents the CPU's |    |                                             
#                            |                                      |  |  | interface to the L1D cache             |  |    | interface to the L1I cache             |    |
#                            |                                      |  |  |________________________________________|  |    |________________________________________|    |
#                            |                                      |  |___________________|__________________________|                        |                         |
#                            | Link                                 |______________________|___________________________________________________|_________________________|
#                            |                                                             |                                                   | 
#                            |                                                             |                                                   | 
#                            |                                                             | Link                                              | Link
#                            |                                                             |                                                   |
#                          __|_____________________________________________________________|___________________________________________________|__
#                          | Component: cache_bus                                                                                                |
#                          | Type: memHierarchy.Bus                                                                                              |
#                          | Because there can only be 1 link to the memory interface of the L2 cache, and we have 3 caches attempting to        |
#                          | we use this bus to collect the 3 links from the caches into a single link to the L2 cache.                          |
#                          |_____________________________________________________________________________________________________________________|
#                                                                                          |
#                                                                                          | Link
#                                                               ___________________________|___________________________
#                                                               | Component: cpu_l2cache                              |
#                                                               | Type: memHierarchy.Cache                            |
#                                                               | This component represents the L2 cache in the model |
#                                                               |_____________________________________________________|
#                                                                                          |
#                                                                                          | Link
#                                                               ___________________________|___________________________                           |     
#                                                               | Component: cpu_l3cache                              |
#                                                               | Type: memHierarchy.Cache                            |
#                                                               | This component represents the L3 cache in the model |
#                                                               |_____________________________________________________|
#
# This example relies on several environmental variables, which allow for costumization. A full list of 
# the available customizations, including default values, is provided below. Note that the default values 
# may be specific to this example, and therefore differ from the default values built into the Vanadis source.
# 
# VANADIS_ISA
#   The ISA the CPU will use.
#   Possible Values: MIPSEL, RISCV64
#   Default: RISCV64
# VANADIS_LOADER_MODE
#   Describes the operation of the loader for the vanadis decoder
#   Possible Values: 0 (use LRU, which is more accurate), 1 (use an infinite cache, which allows for a faster simulation)  
#   Default: 0 (LRU)
# VANADIS_VERBOSE
#   Defines the verbosity of the output from multiple components. 
#   Possible Values: integers >= 0, higher numbers indicate more verbose output
#   Default: 0
# VANADIS_OS_VERBOSE
#   Defines the verbosity of the output for the vanadis.vanadisNodeOS componenent
#   Possible Values: integers >= 0, higher numbers indicate more verbose output
#   Default: VANADIS_VERBOSE
# VANADIS_PIPE_TRACE
#   Defines the loction of the file where Vanadis will record the pipeline activity trace
#   Possible Values: a valid file path, or unspecified
#   Default: unspecified (no trace data will be recorded)
# VANADIS_LSQ_LD_ENTRIES
#   Defines the maximum number of loads allowed by the load store queue
#   Possible Values: integers >= 0
#   Default: 16
# VANADIS_LSQ_ST_ENTRIES
#   Defines the maximum number of stores permitted in the load store queue
#   Possible Values: integers >= 0
#   Default: 8
# VANADIS_ROB_SLOTS
#   Defines the number of slots in the CPU's reorder buffer
#   Possible Values: integers >= 0
#   Default: 64
# VANADIS_RETIRES_PER_CYCLE
#   Defines the number of instruction retires per cycle
#   Possible Values: integers >= 0
#   Default: 4
# VANADIS_ISSUES_PER_CYCLE
#   Defines the number instruction issues per cycle
#   Possible Values: integers >= 0
#   Default: 4
# VANADIS_DECODES_PER_CYCLE
#   Defines the number of instruction decodes per cycle
#   Possible values: integers >= 0
#   Default: 4
# VANADIS_INTEGER_ARITH_CYCLES
#   Defines the number of cycles per instruction the CPU requires for performing integer arithmetic
#   Possible values: integers >= 0
#   Default: 2
# VANADIS_INTEGER_ARITH_UNITS
#   Defines the number of integer arithmetic units
#   Possible values: integers >= 0
#   Default: 2
# VANADIS_FP_ARITH_CYCLES
#   Defines the number of cycles per instruction the CPU requires for performing floating piont arithmetic
#   Possible values: integers >= 0
#   Default: 8 
# VANADIS_FP_ARITH_UNITS
#   Defines the number of floating point division units
#   Possible values: integers >= 0
#   Default: 2
# VANADIS_BRANCH_ARITH_CYCLES
#   Defines the number of cycles per branch
#   Possible Values: integers >= 0
#   Default: 2
# VANADIS_CPU_CLOCK - default: 2.3GHz
#   Defines the core clock frequency for the CPU and memory controller, as well as the cache and bus frequencies
#   Possible values: String values indicating a valid frequency
#   Default: 2.3 GHz
# VANADIS_NUM_CORES
#   Defines the number of cores that can request OS services via a link
#   Possible values: integers > 0
#   Default: 1
# VANADIS_NUM_HW_THREADS
#   Defines the number of hardware threads per CPU core. 
#   Possible Values: integers >= 0
#   Default: 1
# VANADIS_CPU_ELEMENT_NAME - default: dbg_VanadisCPU
#   Defines the type of CPU to use in the model
#   Possible Values: dbg_VanadisCPU, VanadisCPU
#   Default: dbg_VanadisCPU
import os
import sst

mh_debug_level=10
mh_debug=0
dbgAddr="0"
stopDbg="0"

checkpointDir = ""
checkpoint = ""

pythonDebug=False

vanadis_isa = os.getenv("VANADIS_ISA", "RISCV64")
isa=vanadis_isa.lower()

loader_mode = os.getenv("VANADIS_LOADER_MODE", "0")

testDir="basic-io"
exe = "hello-world"

physMemSize = "4GiB"

tlbType = "simpleTLB"
mmuType = "simpleMMU"

# Tell SST what statistics handling we want
sst.setStatisticLoadLevel(4)
sst.setStatisticOutput("sst.statOutputConsole")

# The following retrieves the configuration values documented above from environment values,
# substituting defaults where the environment variables have not been defined.
full_exe_name = "../tests/small/basic-io/hello-world/riscv64/hello-world"
exe_name= full_exe_name.split("/")[-1]

verbosity = int(os.getenv("VANADIS_VERBOSE", 0))
os_verbosity = os.getenv("VANADIS_OS_VERBOSE", verbosity)
pipe_trace_file = os.getenv("VANADIS_PIPE_TRACE", "")
lsq_ld_entries = os.getenv("VANADIS_LSQ_LD_ENTRIES", 16)
lsq_st_entries = os.getenv("VANADIS_LSQ_ST_ENTRIES", 8)

rob_slots = os.getenv("VANADIS_ROB_SLOTS", 64)
retires_per_cycle = os.getenv("VANADIS_RETIRES_PER_CYCLE", 4)
issues_per_cycle = os.getenv("VANADIS_ISSUES_PER_CYCLE", 4)
decodes_per_cycle = os.getenv("VANADIS_DECODES_PER_CYCLE", 4)

integer_arith_cycles = int(os.getenv("VANADIS_INTEGER_ARITH_CYCLES", 2))
integer_arith_units = int(os.getenv("VANADIS_INTEGER_ARITH_UNITS", 2))
fp_arith_cycles = int(os.getenv("VANADIS_FP_ARITH_CYCLES", 8))
fp_arith_units = int(os.getenv("VANADIS_FP_ARITH_UNITS", 2))
branch_arith_cycles = int(os.getenv("VANADIS_BRANCH_ARITH_CYCLES", 2))

cpu_clock = os.getenv("VANADIS_CPU_CLOCK", "2.3GHz")

numCpus = int(os.getenv("VANADIS_NUM_CORES", 1))
numThreads = int(os.getenv("VANADIS_NUM_HW_THREADS", 1))

vanadis_cpu_type = "vanadis."
vanadis_cpu_type += os.getenv("VANADIS_CPU_ELEMENT_NAME","dbg_VanadisCPU")

app_params = {"argc": 1}

vanadis_decoder = "vanadis.Vanadis" + vanadis_isa + "Decoder"
vanadis_os_hdlr = "vanadis.Vanadis" + vanadis_isa + "OSHandler"

protocol="MESI"

# Define the configuration parameters for the various componenets and subcomponents in the mode.
# OS related params
osParams = {
    "processDebugLevel" : 0,
    "dbgLevel" : os_verbosity,
    "dbgMask" : 8,
    "cores" : numCpus,
    "hardwareThreadCount" : numThreads,
    "page_size"  : 4096,
    "physMemSize" : physMemSize,
    "useMMU" : True,
    "checkpointDir" : checkpointDir,
    "checkpoint" : checkpoint
}

# Parameters for the Node OS's L1 cache
osl1cacheParams = {
    "access_latency_cycles" : "2",
    "cache_frequency" : cpu_clock,
    "replacement_policy" : "lru",
    "coherence_protocol" : protocol,
    "associativity" : "8",
    "cache_line_size" : "64",
    "cache_size" : "32 KB",
    "L1" : "1",
    "debug" : mh_debug,
    "debug_level" : mh_debug_level,
}

# Parameters for the memory management unit
mmuParams = {
    "debug_level": 0,
    "num_cores": numCpus,
    "num_threads": numThreads,
    "page_size": 4096,
}

# Memory router parameters
memRtrParams ={
      "xbar_bw" : "1GB/s",
      "link_bw" : "1GB/s",
      "input_buf_size" : "2KB",
      "num_ports" : str(numCpus+2),
      "flit_size" : "72B",
      "output_buf_size" : "2KB",
      "id" : "0",
      "topology" : "merlin.singlerouter"
}

# Directory control parameters
dirCtrlParams = {
      "coherence_protocol" : protocol,
      "entry_cache_size" : "1024",
      "debug" : mh_debug,
      "debug_level" : mh_debug_level,
      "addr_range_start" : "0x0",
      "addr_range_end" : "0xFFFFFFFF"
}

# Dorectpru NIC parameters
dirNicParams = {
      "network_bw" : "25GB/s",
      "group" : 2,
}

# Memory control parameters
memCtrlParams = {
      "clock" : cpu_clock,
      "backend.mem_size" : physMemSize,
      "backing" : "malloc",
      "initBacking": 1,
      "addr_range_start": 0,
      "addr_range_end": 0xffffffff,
      "debug_level" : mh_debug_level,
      "debug" : mh_debug,
      "checkpointDir" : checkpointDir,
      "checkpoint" : checkpoint
}

# Memory parameters
memParams = {
      "mem_size" : "4GiB",
      "access_time" : "1 ns"
}

# CPU related params
tlbParams = {
    "debug_level": 0,
    "hitLatency": 1,
    "num_hardware_threads": numThreads,
    "num_tlb_entries_per_thread": 64,
    "tlb_set_size": 4,
}

# TLB wrapper parameters
tlbWrapperParams = {
    "debug_level": 0,
}

# Node OS's decoder parameters
decoderParams = {
    "loader_mode" : loader_mode,
    "uop_cache_entries" : 1536,
    "predecode_cache_entries" : 4
}

osHdlrParams = { }

# Branch predictor parameters
branchPredParams = {
    "branch_entries" : 32
}

# CPU parameters
cpuParams = {
    "clock" : cpu_clock,
    "verbose" : verbosity,
    "hardware_threads": numThreads,
    "physical_fp_registers" : 168 * numThreads,
    "physical_integer_registers" : 180 * numThreads,
    "integer_arith_cycles" : integer_arith_cycles,
    "integer_arith_units" : integer_arith_units,
    "fp_arith_cycles" : fp_arith_cycles,
    "fp_arith_units" : fp_arith_units,
    "branch_unit_cycles" : branch_arith_cycles,
    "print_int_reg" : False,
    "print_fp_reg" : False,
    "pipeline_trace_file" : pipe_trace_file,
    "reorder_slots" : rob_slots,
    "decodes_per_cycle" : decodes_per_cycle,
    "issues_per_cycle" :  issues_per_cycle,
    "retires_per_cycle" : retires_per_cycle,
    "pause_when_retire_address" : os.getenv("VANADIS_HALT_AT_ADDRESS", 0),
    "start_verbose_when_issue_address": dbgAddr,
    "stop_verbose_when_retire_address": stopDbg,
    "print_rob" : False,
    "checkpointDir" : checkpointDir,
    "checkpoint" : checkpoint
}

# Load store queue parameters
lsqParams = {
    "verbose" : verbosity,
    "address_mask" : 0xFFFFFFFF,
    "max_stores" : lsq_st_entries,
    "max_loads" : lsq_ld_entries,
}

# L1 D cache parameters
l1dcacheParams = {
    "access_latency_cycles" : "2",
    "cache_frequency" : cpu_clock,
    "replacement_policy" : "lru",
    "coherence_protocol" : protocol,
    "associativity" : "8",
    "cache_line_size" : "64",
    "cache_size" : "32 KB",
    "L1" : "1",
    "debug" : mh_debug,
    "debug_level" : mh_debug_level,
}

# L1 I cache parameters
l1icacheParams = {
    "access_latency_cycles" : "2",
    "cache_frequency" : cpu_clock,
    "replacement_policy" : "lru",
    "coherence_protocol" : protocol,
    "associativity" : "8",
    "cache_line_size" : "64",
    "cache_size" : "32 KB",
    "prefetcher" : "cassini.NextBlockPrefetcher",
    "prefetcher.reach" : 1,
    "L1" : "1",
    "debug" : mh_debug,
    "debug_level" : mh_debug_level,
}

# L2 cache parameters
l2cacheParams = {
    "access_latency_cycles" : "14",
    "cache_frequency" : cpu_clock,
    "replacement_policy" : "lru",
    "coherence_protocol" : protocol,
    "associativity" : "16",
    "cache_line_size" : "64",
    "cache_size" : "1MB",
    "mshr_latency_cycles": 3,
    "debug" : mh_debug,
    "debug_level" : mh_debug_level,
}

# Bus paremeters (connecting L1 caches to L2 cache)
busParams = { 
    "bus_frequency" : cpu_clock, 
}

# l2 cache  memory link parameters
l2memLinkParams = { 
    "group" : 1,
    "network_bw" : "25GB/s" 
}

# The parameters for processes are sub-parameters, and need to be keyed differently. 
# This function accomplishes prefixing the keys.
def addParamsPrefix(prefix,params):
    ret = {}
    for key, value in params.items():
        ret[ prefix + "." + key] = value

    return ret

# This creates the hardware Node and OS for the model. It has its own memory managument 
# unit and memory interface. It is also connected to its own L1D cache
node_os = sst.Component("os", "vanadis.VanadisNodeOS")
node_os.addParams(osParams)

# Define the application the CPU will run in this simulation and the number of threads it will use
processList = ( 
    ( 1, {
        "env_count" : 1,
        "env0" : "OMP_NUM_THREADS={}".format(numCpus*numThreads),
        "exe" : full_exe_name,
        "arg0" : exe_name,
    } ),
)

processList[0][1].update(app_params)

# Create the parameters for the processes on the node. The process parameters were created 
# above, but to be successfully be applied, they need to be prefixed in the node_os params 
# correctly, which ties the parameters to a specific process on the node. In this example, 
# we only have 1 process, so all parameters are prefixed with "process0."
num=0
for i,process in processList: 
    for y in range(i):
        node_os.addParams( addParamsPrefix( "process" + str(num), process ) )
        num+=1

# Create the memory management unit sub component for the node. 
node_os_mmu = node_os.setSubComponent( "mmu", "mmu." + mmuType )
node_os_mmu.addParams(mmuParams)

# Create the memory interface from the OS to the cache
node_os_mem_if = node_os.setSubComponent( "mem_interface", "memHierarchy.standardInterface" )

# Create an L1 data cache for the node OS, and connects it to the OS component and the
# memory representation
os_cache = sst.Component("node_os.cache", "memHierarchy.Cache")
os_cache.addParams(osl1cacheParams)
os_cache_2_cpu = os_cache.setSubComponent("cpulink", "memHierarchy.MemLink")
os_cache_2_mem = os_cache.setSubComponent("memlink", "memHierarchy.MemNIC")
os_cache_2_mem.addParams( l2memLinkParams )

# node memory router 
comp_chiprtr = sst.Component("chiprtr", "merlin.hr_router")
comp_chiprtr.addParams(memRtrParams)
comp_chiprtr.setSubComponent("topology","merlin.singlerouter")

# node directory controller
dirctrl = sst.Component("dirctrl", "memHierarchy.DirectoryController")
dirctrl.addParams(dirCtrlParams)

# Define how the directory controller will connect to memory
dirtoM = dirctrl.setSubComponent("memlink", "memHierarchy.MemLink")

# Define how the directory controller will connect to the CPU
dirNIC = dirctrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
dirNIC.addParams(dirNicParams)

# node memory controller
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams( memCtrlParams )

# node memory controller port to directory controller
memToDir = memctrl.setSubComponent("cpulink", "memHierarchy.MemLink")

# node memory controller backend 
memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams(memParams)

# Directory controller to memory router
link_dir_2_rtr = sst.Link("link_dir_2_rtr")
link_dir_2_rtr.connect( (comp_chiprtr, "port"+str(numCpus), "1ns"), (dirNIC, "port", "1ns") )
link_dir_2_rtr.setNoCut()

# Directory controller to memory controller 
link_dir_2_mem = sst.Link("link_dir_2_mem")
link_dir_2_mem.connect( (dirtoM, "port", "1ns"), (memToDir, "port", "1ns") )
link_dir_2_mem.setNoCut()

# connect the OS's TLB ot the cache
link_os_cache_link = sst.Link("link_os_cache_link")
link_os_cache_link.connect( (node_os_mem_if, "port", "1ns"), (os_cache_2_cpu, "port", "1ns") )
link_os_cache_link.setNoCut()

# connect the os cache to the router
os_cache_2_rtr = sst.Link("os_cache_2_rtr")
os_cache_2_rtr.connect( (os_cache_2_mem, "port", "1ns"), (comp_chiprtr, "port"+str(numCpus+1), "1ns") )
os_cache_2_rtr.setNoCut()

# Build a CPU for the Node
nodeId = 0

prefix="node" + str(nodeId) + ".cpu0"

# Build and configure the CPU, and enable statistics on it
cpu = sst.Component(prefix, vanadis_cpu_type)
cpu.addParams( cpuParams )
cpu.addParam( "core_id", 0 )
cpu.enableAllStatistics()

# Build and configure the decoder subcomponent of the CPU
decode = cpu.setSubComponent( "decoder0", vanadis_decoder )
decode.addParams( decoderParams )
decode.enableAllStatistics()

# Build and configure the OS handler subcomponent of the decoder
os_hdlr = decode.setSubComponent( "os_handler", vanadis_os_hdlr )
os_hdlr.addParams( osHdlrParams )

# Create the branch predictor sub component of the decoder
branch_pred = decode.setSubComponent( "branch_unit", "vanadis.VanadisBasicBranchUnit" )
branch_pred.addParams( branchPredParams )
branch_pred.enableAllStatistics()

# Build and configure the CPU's load store queue
cpu_lsq = cpu.setSubComponent( "lsq", "vanadis.VanadisBasicLoadStoreQueue" )
cpu_lsq.addParams(lsqParams)
cpu_lsq.enableAllStatistics()

# Create the memory interface on the CPU's load store queue, which defines 
# how the load store queue connects to memory
cpuDcacheIf = cpu_lsq.setSubComponent( "memory_interface", "memHierarchy.standardInterface" )

# Creates the interface for the CPU to connect to the instruction cache (towards memory)
cpuIcacheIf = cpu.setSubComponent( "mem_interface_inst", "memHierarchy.standardInterface" )

# Creates and configures the L1D cache for the CPU
cpu_l1dcache = sst.Component(prefix + ".l1dcache", "memHierarchy.Cache")
cpu_l1dcache.addParams( l1dcacheParams )

# Define the interface between the L1D cache and the CPU
l1dcache_2_cpu     = cpu_l1dcache.setSubComponent("cpulink", "memHierarchy.MemLink")
# Define the L1D cache's interface towards memory, which will connect to the bus, which
# in turn, connects to tje L21 cache.
l1dcache_2_l2cache = cpu_l1dcache.setSubComponent("memlink", "memHierarchy.MemLink")

# Create the L1 I cache 
cpu_l1icache = sst.Component( prefix + ".l1icache", "memHierarchy.Cache")
cpu_l1icache.addParams( l1icacheParams )

# Define how the L1 I cache will connect to the CPU
l1icache_2_cpu     = cpu_l1icache.setSubComponent("cpulink", "memHierarchy.MemLink")
# Define how the L1 I cache will connect towards memory; i.e., how the L1 I cache 
# connects to the bus, to then connect to the L2 cache
l1icache_2_l2cache = cpu_l1icache.setSubComponent("memlink", "memHierarchy.MemLink")

# Creates and configures the L2 cache
cpu_l2cache = sst.Component(prefix+".l2cache", "memHierarchy.Cache")
cpu_l2cache.addParams( l2cacheParams )

# Defines how the L2 cache will connect towards the CPU. Because there are multiple
# L1 caches between the L2 cache the CPU that the L2 cache needs to connect to, we use
# a bus in between them, which will be what the link actually connects to.
l2cache_2_l1caches = cpu_l2cache.setSubComponent("cpulink", "memHierarchy.MemLink")

# Defines how the L2 cache will connect towards memory - which in this case is 
# a connection to the L3 cache
l2cache_2_l3cache = cpu_l2cache.setSubComponent("memlink", "memHierarchy.MemLink")
l2cache_2_l3cache.addParams( l2memLinkParams )

# Create and configure the L3 cache
cpu_l3cache = sst.Component(prefix+".l3cache", "memHierarchy.Cache")
cpu_l3cache.addParams({
    "access_latency_cycles" : "14",
    "cache_frequency" : cpu_clock,
    "replacement_policy" : "lru",
    "coherence_protocol" : protocol,
    "associativity" : "16",
    "cache_line_size" : "64",
    "cache_size" : "1MB",
    "mshr_latency_cycles": 3,
    "debug" : mh_debug,
    "debug_level" : mh_debug_level,
})

# Define how the L3 cache will connect towards the CPU. In this case the next component
# closer the the CPU is the L2 cache. Because there's only 1 L2 cache, the L3 cache and L2
# cache will be linked directly, instead of through a bus.
l3cache_2_l2cache = cpu_l3cache.setSubComponent("cpulink", "memHierarchy.MemLink")

# Define how the L3 cache will connect towards memory. The L3 cache is the final 
# cache, so it will connect directly to memory.
l3cache_2_mem = cpu_l3cache.setSubComponent("memlink", "memHierarchy.MemNIC")
l3cache_2_mem.addParams({ 
    "group" : 1,
    "network_bw" : "25GB/s" 
})

# Create and configure the bus for connecting the L1 caches to the L2 cache
cache_bus = sst.Component(prefix+".bus", "memHierarchy.Bus")
cache_bus.addParams(busParams)

# Create and configure CPU data TLB
dtlbWrapper = sst.Component(prefix+".dtlb", "mmu.tlb_wrapper")
dtlbWrapper.addParams(tlbWrapperParams)
dtlb = dtlbWrapper.setSubComponent("tlb", "mmu." + tlbType )
dtlb.addParams(tlbParams)

# Create and configure the CPU instruction TLB
itlbWrapper = sst.Component(prefix+".itlb", "mmu.tlb_wrapper")
itlbWrapper.addParams(tlbWrapperParams)
itlbWrapper.addParam("exe",True)
itlb = itlbWrapper.setSubComponent("tlb", "mmu." + tlbType )
itlb.addParams(tlbParams)

# Connect the CPU data TLB to the CPU D cache
link_cpu_dtlb_link = sst.Link(prefix+".link_cpu_dtlb_link")
link_cpu_dtlb_link.connect( (cpuDcacheIf, "port", "1ns"), (dtlbWrapper, "cpu_if", "1ns") )
link_cpu_dtlb_link.setNoCut()

# Connect the data TLB to the L1 D cache
link_cpu_l1dcache_link = sst.Link(prefix+".link_cpu_l1dcache_link")
link_cpu_l1dcache_link.connect( (dtlbWrapper, "cache_if", "1ns"), (l1dcache_2_cpu, "port", "1ns") )
link_cpu_l1dcache_link.setNoCut()

# Connect the CPU TLB to the CPU I cache interface
link_cpu_itlb_link = sst.Link(prefix+".link_cpu_itlb_link")
link_cpu_itlb_link.connect( (cpuIcacheIf, "port", "1ns"), (itlbWrapper, "cpu_if", "1ns") )
link_cpu_itlb_link.setNoCut()

# Connect the I TLB to teh L1 I cache
link_cpu_l1icache_link = sst.Link(prefix+".link_cpu_l1icache_link")
link_cpu_l1icache_link.connect( (itlbWrapper, "cache_if", "1ns"), (l1icache_2_cpu, "port", "1ns") )
link_cpu_l1icache_link.setNoCut()

# Connect the L2 D cache to the cache bus
link_l1dcache_l2cache_link = sst.Link(prefix+".link_l1dcache_l2cache_link")
link_l1dcache_l2cache_link.connect( (l1dcache_2_l2cache, "port", "1ns"), (cache_bus, "high_network_0", "1ns") )
link_l1dcache_l2cache_link.setNoCut()

# Connect the L1 I cache to teh cache bus
link_l1icache_l2cache_link = sst.Link(prefix+".link_l1icache_l2cache_link")
link_l1icache_l2cache_link.connect( (l1icache_2_l2cache, "port", "1ns"), (cache_bus, "high_network_1", "1ns") )
link_l1icache_l2cache_link.setNoCut()

# Connect the cache bus to the L2 cache
link_bus_l2cache_link = sst.Link(prefix+".link_bus_l2cache_link")
link_bus_l2cache_link.connect( (cache_bus, "low_network_0", "1ns"), (l2cache_2_l1caches, "port", "1ns") )
link_bus_l2cache_link.setNoCut()

# Connect the Node OS's MMU to the data TLB
link_mmu_dtlb_link = sst.Link(prefix + ".link_mmu_dtlb_link")
link_mmu_dtlb_link.connect( (node_os_mmu, "core0.dtlb", "1ns"),  (dtlb, "mmu", "1ns") )

# Connect the Node OS's MMU to the instruction TLB
link_mmu_itlb_link = sst.Link(prefix + ".link_mmu_itlb_link")
link_mmu_itlb_link.connect( (node_os_mmu, "core0.itlb", "1ns"), (itlb, "mmu", "1ns") )

# Connect the CPU's os handler to the Node OS
link_core_os_link = sst.Link(prefix + ".link_core_os_link")
link_core_os_link.connect( (cpu, "os_link", "5ns"), (node_os, "core0", "5ns") )

# Connect the L2 cache to the L3 cache
link_l2cache_l3cache_link = sst.Link(prefix + ".link_l2cache_2_l3cache")
link_l2cache_l3cache_link.connect((l2cache_2_l3cache, "port", "1ns"), (l3cache_2_l2cache, "port", "1ns"))
link_l2cache_l3cache_link.setNoCut()

# Connect the L3 to memory router
link_l3cache_2_rtr = sst.Link(prefix + ".link_l2cache_2_rtr")
link_l3cache_2_rtr.connect( (l3cache_2_mem, "port", "1ns"), (comp_chiprtr, "port0", "1ns") )
