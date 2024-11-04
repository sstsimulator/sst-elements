import os
import sst
import array

mh_debug_level=10
mh_debug=0
dbgAddr=0
stopDbg=0

pythonDebug=False

vanadis_isa = os.getenv("VANADIS_ISA", "MIPS")
isa="mipsel"
vanadis_isa = os.getenv("VANADIS_ISA", "RISCV64")
isa="riscv64"

loader_mode = os.getenv("VANADIS_LOADER_MODE", "0")

testDir="basic-io"
exe = "hello-world"
#exe = "hello-world-cpp"
#exe = "openat"
#exe = "printf-check"
#exe = "read-write"
#exe = "fread-fwrite"
#exe = "unlink"
#exe = "unlinkat"
#exe = "lseek"

#testDir = "basic-math"
#exe = "sqrt-double"
#exe = "sqrt-float"

#testDir = "basic-ops"
#exe = "test-branch"
#exe = "test-shift"

#testDir = "misc"
#exe = "mt-dgemm"
#exe = "stream"
#exe = "stream-fortran"
#exe = "gettime"
#exe = "splitLoad"
#exe = "fork"
#exe = "clone"
#exe = "pthread"
#exe = "openmp"
#exe = "openmp2"
#exe = "uname"
#exe = "mem-test"
#exe = "checkpoint"

physMemSize = "4GiB"

tlbType = "simpleTLB"
mmuType = "simpleMMU"

backingType = "timingDRAM" ## simpleMem, simpleDRAM, timingDRAM

# Define SST core options
sst.setProgramOption("timebase", "1ps")

# Tell SST what statistics handling we want
sst.setStatisticLoadLevel(100)
sst.setStatisticOutput("sst.statOutputConsole")

full_exe_name = os.getenv("VANADIS_EXE", "../tests/small/" + testDir + "/" + exe +  "/" + isa + "/" + exe )
exe_name= full_exe_name.split("/")[-1]

verbosity = int(os.getenv("VANADIS_VERBOSE", 0))
os_verbosity = os.getenv("VANADIS_OS_VERBOSE", verbosity)
pipe_trace_file = os.getenv("VANADIS_PIPE_TRACE", "")
lsq_ld_entries = os.getenv("VANADIS_LSQ_LD_ENTRIES", 16)
lsq_st_entries = os.getenv("VANADIS_LSQ_ST_ENTRIES", 8)

rob_slots = os.getenv("VANADIS_ROB_SLOTS", 96)
retires_per_cycle = os.getenv("VANADIS_RETIRES_PER_CYCLE", 3)
issues_per_cycle = os.getenv("VANADIS_ISSUES_PER_CYCLE", 3)
decodes_per_cycle = os.getenv("VANADIS_DECODES_PER_CYCLE", 3)
fetches_per_cycle = os.getenv("VANADIS_FETCHES_PER_CYCLE", 8)

integer_arith_cycles = int(os.getenv("VANADIS_INTEGER_ARITH_CYCLES", 3))
integer_arith_units = int(os.getenv("VANADIS_INTEGER_ARITH_UNITS", 3))
fp_arith_cycles = int(os.getenv("VANADIS_FP_ARITH_CYCLES", 4))
fp_arith_units = int(os.getenv("VANADIS_FP_ARITH_UNITS", 1))
branch_arith_cycles = int(os.getenv("VANADIS_BRANCH_ARITH_CYCLES", 2))

cpu_clock = os.getenv("VANADIS_CPU_CLOCK", "3.2GHz")
mem_clock = os.getenv("VANADIS_MEM_CLOCK", "1.0GHz")

numCpus = int(os.getenv("VANADIS_NUM_CORES", 1))
numThreads = int(os.getenv("VANADIS_NUM_HW_THREADS", 1))
numMemories = int(os.getenv("VANADIS_NUM_MEMORIES", 2))

vanadis_cpu_type = "vanadis."
vanadis_cpu_type += os.getenv("VANADIS_CPU_ELEMENT_NAME","dbg_VanadisCPU")

# cpu and memory locations; dc's located at mem nodes
cpu_node_list = array.array('i', [1])
mem_node_list = array.array('i', [0,2])

if( len(cpu_node_list) != numCpus ):
    raise ValueError("Not enough enumerated cpus...Exiting...")

if( len(mem_node_list) != numMemories ):
    raise ValueError("Not enough enumerated memories...Exiting...")

## mesh
mesh_stops_x    = 3
mesh_stops_y    = 2

mesh_clock          = 3200
ctrl_mesh_flit      = 8         # msg size in B
data_mesh_flit      = 36        # msg size in B
mesh_link_latency   = "100ps"
ctrl_mesh_link_bw   = str( (mesh_clock * 1000 * 1000 * ctrl_mesh_flit) ) + "B/s"
data_mesh_link_bw   = str( (mesh_clock * 1000 * 1000 * data_mesh_flit) ) + "B/s"

ctrl_network_buffers = "32B"
data_network_buffers = "288B"

ctrl_network_params = {
        "link_bw" : ctrl_mesh_link_bw,
        "flit_size" : str(ctrl_mesh_flit) + "B",
        "input_buf_size" : ctrl_network_buffers,
}

data_network_params = {
        "link_bw" : data_mesh_link_bw,
        "flit_size" : str(data_mesh_flit) + "B",
        "input_buf_size" : data_network_buffers,
        "port_priority_equal" : 1,
}

if (verbosity > 0):
    print("Verbosity: " + str(verbosity) + " -> loading Vanadis CPU type: " + vanadis_cpu_type)
    print("Auto-clock syscalls: " + str(auto_clock_sys))
#       vanadis_cpu_type = "vanadisdbg.VanadisCPU"

app_args = os.getenv("VANADIS_EXE_ARGS", "")

app_params = {}
if app_args != "":
    app_args_list = app_args.split(" ")
    # We have a plus 1 because the executable name is arg0
    app_args_count = len( app_args_list ) + 1

    app_params["argc"] = app_args_count

    if (verbosity > 0):
        print("Identified " + str(app_args_count) + " application arguments, adding to input parameters.")
    arg_start = 1
    for next_arg in app_args_list:
        if (verbosity > 0):
            print("arg" + str(arg_start) + " = " + next_arg)
        app_params["arg" + str(arg_start)] = next_arg
        arg_start = arg_start + 1
else:
    app_params["argc"] = 1
    if (verbosity > 0):
        print("No application arguments found, continuing with argc=1")

vanadis_decoder = "vanadis.Vanadis" + vanadis_isa + "Decoder"
vanadis_os_hdlr = "vanadis.Vanadis" + vanadis_isa + "OSHandler"


protocol="MESI"

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

processList = (
    ( 1, {
        "env_count" : 1,
        "env0" : "OMP_NUM_THREADS={}".format(numCpus*numThreads),
        "exe" : full_exe_name,
        "arg0" : exe_name,
    } ),
)

processList[0][1].update(app_params)

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

mmuParams = {
    "debug_level": 0,
    "num_cores": numCpus,
    "num_threads": numThreads,
    "page_size": 4096,
}

memParams = {
      "mem_size" : "4GiB",
      "access_time" : "1 ns"
}

# DDR3-1600
# From FASEDTargetConfigs.scala
### DDR3 - First-Ready FCFS models
###     Window size 8, Queue depth 8, Max reads 32, Max writes 32
### L2
###     4096 ways, 8 sets
# From TargetConfigs.scala
### Periphery bus 3200, Memory bus 1000

simpleTimingParams = {
    "max_requests_per_cycle" : 1,
    "mem_size" : "4GiB",
    "tCAS" : 8,
    "tRCD" : 8,
    "tRP" : 8,
    "cycle_time" : "1.25ns",
    "row_size" : "2KiB",
    "row_policy" : "open",
    "debug_mask" : mh_debug,
    "debug_level" : mh_debug_level
}

timingParams = {
    "id" : 0,
    "addrMapper" : "memHierarchy.roundRobinAddrMapper",
    "addrMapper.interleave_size" : "64B",
    "addrMapper.row_size" : "2KiB",
    "clock" : mem_clock,
    "mem_size" : "4GiB",
    "channels" : 3,
    "channel.numRanks" : 4,
    "channel.rank.numBanks" : 8,
    "channel.transaction_Q_size" : 32,
    "channel.rank.bank.CL" : 8,
    "channel.rank.bank.CL_WR" : 6,
    "channel.rank.bank.RCD" : 8,
    "channel.rank.bank.TRP" : 8,
    "channel.rank.bank.dataCycles" : 2,
    "channel.rank.bank.pagePolicy" : "memHierarchy.simplePagePolicy",
    "channel.rank.bank.transactionQ" : "memHierarchy.reorderTransactionQ",
    "channel.rank.bank.pagePolicy.close" : 0,
    "printconfig" : 0,
    "channel.printconfig" : 0,
    "channel.rank.printconfig" : 0,
    "channel.rank.bank.printconfig" : 0,
}

# CPU related params
tlbParams = {
    "debug_level": 0,
    "hitLatency": 1,
    "num_hardware_threads": numThreads,
    "num_tlb_entries_per_thread": 512,
    "tlb_set_size": 1,
}

tlbWrapperParams = {
    "debug_level": 0,
}

decoderParams = {
    "loader_mode" : loader_mode,
    "uop_cache_entries" : 1536,
    "predecode_cache_entries" : 4
}

osHdlrParams = { }

branchPredParams = {
    "branch_entries" : 32
}

## https://docs.boom-core.org/_/downloads/en/stable/pdf/
## https://github.com/riscv-boom/riscv-boom/blob/master/src/main/scala/common/config-mixins.scala
## There are two queues: the Load Queue (LDQ), and the Store Queue (STQ)
cpuParams = {
    "clock" : cpu_clock,
    "verbose" : verbosity,
    "hardware_threads": numThreads,
    "physical_fp_registers" : 96,
    "physical_int_registers" : 100,
    "integer_arith_cycles" : integer_arith_cycles,
    "integer_arith_units" : integer_arith_units,
    "fp_arith_cycles" : fp_arith_cycles,
    "fp_arith_units" : fp_arith_units,
    "integer_div_units" : 3,
    "integer_div_cycles" : 8,   ## this is probably variable latency
    "fp_div_units" : 1,
    "fp_div_cycles" : 48,
    "branch_units" : 4,    ## pretty sure all int FUs can do branches
    "branch_unit_cycles" : branch_arith_cycles,
    "print_int_reg" : False,
    "print_fp_reg" : False,
    "pipeline_trace_file" : pipe_trace_file,
    "reorder_slots" : rob_slots,
    "decodes_per_cycle" : decodes_per_cycle,
    "issues_per_cycle" :  issues_per_cycle,
    "fetches_per_cycle" : fetches_per_cycle,
    "retires_per_cycle" : retires_per_cycle,
    "pause_when_retire_address" : os.getenv("VANADIS_HALT_AT_ADDRESS", 0),
    "start_verbose_when_issue_address": dbgAddr,
    "stop_verbose_when_retire_address": stopDbg,
    "print_rob" : False,
}

lsqParams = {
    "verbose" : verbosity,
    "address_mask" : 0xFFFFFFFF,
    "max_loads" : lsq_ld_entries,
    "max_stores" : lsq_st_entries,
}

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

l2cacheParams = {
    "access_latency_cycles" : "14",
    "cache_frequency" : cpu_clock,
    "replacement_policy" : "lru",
    "coherence_protocol" : protocol,
    "associativity" : "8",
    "cache_line_size" : "64",
    "cache_size" : "4 MB",
    "mshr_latency_cycles": 3,
    "debug" : mh_debug,
    "debug_level" : mh_debug_level,
}

l2_prefetch_params = {
    "reach" : 16,
    "detect_range" : 1
}

busParams = {
    "bus_frequency" : cpu_clock,
}

l2memLinkParams = {
    "group" : 1,
    "debug" : mh_debug,
    "debug_level" : mh_debug_level,
}

class CPU_Builder:
    def __init__(self):
        pass

    # CPU
    def build( self, prefix, nodeId, cpuId ):

        if pythonDebug:
            print("build {}".format(prefix) )

        # CPU
        cpu = sst.Component(prefix, vanadis_cpu_type)
        cpu.addParams( cpuParams )
        cpu.addParam( "core_id", cpuId )
        cpu.enableAllStatistics()

        # CPU.decoder
        for n in range(numThreads):
            decode     = cpu.setSubComponent( "decoder"+str(n), vanadis_decoder )
            decode.addParams( decoderParams )

            decode.enableAllStatistics()

            # CPU.decoder.osHandler
            os_hdlr     = decode.setSubComponent( "os_handler", vanadis_os_hdlr )
            os_hdlr.addParams( osHdlrParams )

            # CPU.decocer.branch_pred
            branch_pred = decode.setSubComponent( "branch_unit", "vanadis.VanadisBasicBranchUnit" )
            branch_pred.addParams( branchPredParams )
            branch_pred.enableAllStatistics()

        # CPU.lsq
        cpu_lsq = cpu.setSubComponent( "lsq", "vanadis.VanadisBasicLoadStoreQueue" )
        cpu_lsq.addParams(lsqParams)
        cpu_lsq.enableAllStatistics()

        # CPU.lsq mem interface which connects to D-cache
        cpuDcacheIf = cpu_lsq.setSubComponent( "memory_interface", "memHierarchy.standardInterface" )

        # CPU.mem interface for I-cache
        cpuIcacheIf = cpu.setSubComponent( "mem_interface_inst", "memHierarchy.standardInterface" )

        # L1 D-cache
        cpu_l1dcache = sst.Component(prefix + ".l1dcache", "memHierarchy.Cache")
        cpu_l1dcache.addParams( l1dcacheParams )

        # L1 I-cache to cpu interface
        l1dcache_2_cpu     = cpu_l1dcache.setSubComponent("cpulink", "memHierarchy.MemLink")
        # L1 I-cache to L2 interface
        l1dcache_2_l2cache = cpu_l1dcache.setSubComponent("memlink", "memHierarchy.MemLink")

        # L2 I-cache
        cpu_l1icache = sst.Component( prefix + ".l1icache", "memHierarchy.Cache")
        cpu_l1icache.addParams( l1icacheParams )

        # L1 I-iache to cpu interface
        l1icache_2_cpu     = cpu_l1icache.setSubComponent("cpulink", "memHierarchy.MemLink")
        # L1 I-cache to L2 interface
        l1icache_2_l2cache = cpu_l1icache.setSubComponent("memlink", "memHierarchy.MemLink")

        # L2 cache
        cpu_l2cache = sst.Component(prefix + ".l2cache", "memHierarchy.Cache")
        cpu_l2cache.addParams( l2cacheParams )
        l2pre = cpu_l2cache.setSubComponent("prefetcher", "cassini.StridePrefetcher")
        l2pre.addParams(l2_prefetch_params)

        # L2 cache cpu interface
        l2cache_2_l1caches = cpu_l2cache.setSubComponent("cpulink", "memHierarchy.MemLink")

        # L2 cache mem interface
        l2cache_2_mem = cpu_l2cache.setSubComponent("memlink", "memHierarchy.MemNICFour")
        l2cache_2_mem.addParams( l2memLinkParams )
        l2data = l2cache_2_mem.setSubComponent("data", "kingsley.linkcontrol")
        l2req  = l2cache_2_mem.setSubComponent("req", "kingsley.linkcontrol")
        l2fwd  = l2cache_2_mem.setSubComponent("fwd", "kingsley.linkcontrol")
        l2ack  = l2cache_2_mem.setSubComponent("ack", "kingsley.linkcontrol")
        l2data.addParams(data_network_params)
        l2req.addParams(ctrl_network_params)
        l2fwd.addParams(ctrl_network_params)
        l2ack.addParams(ctrl_network_params)

        # L1 to L2 buss
        cache_bus = sst.Component(prefix + ".bus", "memHierarchy.Bus")
        cache_bus.addParams(busParams)

        # CPU data TLB
        dtlbWrapper = sst.Component(prefix + ".dtlb", "mmu.tlb_wrapper")
        dtlbWrapper.addParams(tlbWrapperParams)
#        dtlbWrapper.addParam( "debug_level", 0)
        dtlb = dtlbWrapper.setSubComponent("tlb", "mmu." + tlbType );
        dtlb.addParams(tlbParams)

        # CPU instruction TLB
        itlbWrapper = sst.Component(prefix+".itlb", "mmu.tlb_wrapper")
        itlbWrapper.addParams(tlbWrapperParams)
#        itlbWrapper.addParam(    "debug_level", 0)
        itlbWrapper.addParam("exe",True)
        itlb = itlbWrapper.setSubComponent("tlb", "mmu." + tlbType );
        itlb.addParams(tlbParams)

        # CPU (data) -> TLB -> Cache
        link_cpu_dtlb_link = sst.Link(prefix+".link_cpu_dtlb_link")
        link_cpu_dtlb_link.connect( (cpuDcacheIf, "port", "1ns"), (dtlbWrapper, "cpu_if", "1ns") )
        link_cpu_dtlb_link.setNoCut()

        # data TLB -> data L1
        link_cpu_l1dcache_link = sst.Link(prefix+".link_cpu_l1dcache_link")
        link_cpu_l1dcache_link.connect( (dtlbWrapper, "cache_if", "1ns"), (l1dcache_2_cpu, "port", "1ns") )
        link_cpu_l1dcache_link.setNoCut()

        # CPU (instruction) -> TLB -> Cache
        link_cpu_itlb_link = sst.Link(prefix+".link_cpu_itlb_link")
        link_cpu_itlb_link.connect( (cpuIcacheIf, "port", "1ns"), (itlbWrapper, "cpu_if", "1ns") )
        link_cpu_itlb_link.setNoCut()

        # instruction TLB -> instruction L1
        link_cpu_l1icache_link = sst.Link(prefix+".link_cpu_l1icache_link")
        link_cpu_l1icache_link.connect( (itlbWrapper, "cache_if", "1ns"), (l1icache_2_cpu, "port", "1ns") )
        link_cpu_l1icache_link.setNoCut();

        # data L1 -> bus
        link_l1dcache_l2cache_link = sst.Link(prefix+".link_l1dcache_l2cache_link")
        link_l1dcache_l2cache_link.connect( (l1dcache_2_l2cache, "port", "1ns"), (cache_bus, "high_network_0", "1ns") )
        link_l1dcache_l2cache_link.setNoCut()

        # instruction L1 -> bus
        link_l1icache_l2cache_link = sst.Link(prefix+".link_l1icache_l2cache_link")
        link_l1icache_l2cache_link.connect( (l1icache_2_l2cache, "port", "1ns"), (cache_bus, "high_network_1", "1ns") )
        link_l1icache_l2cache_link.setNoCut()

        # BUS to L2 cache
        link_bus_l2cache_link = sst.Link(prefix+".link_bus_l2cache_link")
        link_bus_l2cache_link.connect( (cache_bus, "low_network_0", "1ns"), (l2cache_2_l1caches, "port", "1ns") )
        link_bus_l2cache_link.setNoCut()

        return (cpu, "os_link", "5ns"), (dtlb, "mmu", "1ns"), (itlb, "mmu", "1ns"), \
               (l2req, "rtr_port", mesh_link_latency), (l2ack, "rtr_port", mesh_link_latency), \
               (l2fwd, "rtr_port", mesh_link_latency), (l2data, "rtr_port", mesh_link_latency)


def addParamsPrefix(prefix,params):
    #print( prefix )
    ret = {}
    for key, value in params.items():
        #print( key, value )
        ret[ prefix + "." + key] = value

    #print( ret )
    return ret

# node OS
node_os = sst.Component("os", "vanadis.VanadisNodeOS")
node_os.addParams(osParams)

num=0
for i,process in processList:
    #print( process )
    for y in range(i):
        #print( "process", num )
        node_os.addParams( addParamsPrefix( "process" + str(num), process ) )
        num+=1

if pythonDebug:
    print('total hardware threads ' + str(num) )

# node OS MMU
node_os_mmu = node_os.setSubComponent( "mmu", "mmu." + mmuType )
node_os_mmu.addParams(mmuParams)

# node OS memory interface to L1 data cache
node_os_mem_if = node_os.setSubComponent( "mem_interface", "memHierarchy.standardInterface" )

# node OS l1 data cache
os_cache = sst.Component("node_os.cache", "memHierarchy.Cache")
os_cache.addParams(osl1cacheParams)
os_cache_2_cpu = os_cache.setSubComponent("cpulink", "memHierarchy.MemLink")
os_cache_2_mem = os_cache.setSubComponent("memlink", "memHierarchy.MemNICFour")
os_cache_2_mem.addParams( l2memLinkParams )
os_data = os_cache_2_mem.setSubComponent("data", "kingsley.linkcontrol")
os_req = os_cache_2_mem.setSubComponent("req", "kingsley.linkcontrol")
os_ack = os_cache_2_mem.setSubComponent("ack", "kingsley.linkcontrol")
os_fwd = os_cache_2_mem.setSubComponent("fwd", "kingsley.linkcontrol")
os_data.addParams(data_network_params)
os_req.addParams(ctrl_network_params)
os_fwd.addParams(ctrl_network_params)
os_ack.addParams(ctrl_network_params)

## Memory Building
class MemBuilder:
    def __init__(self, capacity):
        self.mem_capacity = capacity

    def build(self, nodeID, memID):
        if pythonDebug:
            print("Creating DDR controller on node " + str(nodeID) + "...")
            print(" - Capacity: " + str(self.mem_capacity // numMemories) + " per DDR.")

        memctrl = sst.Component("memory" + str(nodeID), "memHierarchy.MemController")
        memctrl.addParams({
            "clock" : mem_clock,
            "backend.mem_size" : physMemSize,
            "backing" : "malloc",
            "initBacking": 1,
            "debug_level" : mh_debug_level,
            "debug" : mh_debug,
            "interleave_size" : "64B",    # Interleave at line granularity between memories
            "interleave_step" : str(numMemories * 64) + "B",
            "addr_range_start" : memID*64,
            "addr_range_end" :  1024*1024*1024 - ((numMemories - memID) * 64) + 63,
        })

        memNIC = memctrl.setSubComponent("cpulink", "memHierarchy.MemNICFour")
        memNIC.addParams({
            "group" : 3,
            "debug_level" : mh_debug_level,
            "debug" : mh_debug,
        })
        memdata = memNIC.setSubComponent("data", "kingsley.linkcontrol")
        memreq = memNIC.setSubComponent("req", "kingsley.linkcontrol")
        memack = memNIC.setSubComponent("ack", "kingsley.linkcontrol")
        memfwd = memNIC.setSubComponent("fwd", "kingsley.linkcontrol")
        memdata.addParams(data_network_params)
        memreq.addParams(ctrl_network_params)
        memfwd.addParams(ctrl_network_params)
        memack.addParams(ctrl_network_params)

        # node memory controller backend
        if backingType == "simpleMem":
            memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
            memory.addParams(memParams)
        elif backingType == "simpleDRAM":
            memory = memctrl.setSubComponent("backend", "memHierarchy.simpleDRAM")
            memory.addParams(simpleTimingParams)
        else:
            memory = memctrl.setSubComponent("backend", "memHierarchy.timingDRAM")
            memory.addParams(timingParams)

        memctrl.enableAllStatistics()
        return (memreq, "rtr_port", mesh_link_latency), (memack, "rtr_port", mesh_link_latency), \
               (memfwd, "rtr_port", mesh_link_latency), (memdata, "rtr_port", mesh_link_latency)

class DCBuilder:

    def build(self, nodeID, memID):
        dirctrl = sst.Component("directory" + str(nodeID), "memHierarchy.DirectoryController")
        dirctrl.addParams({
            "clock" : mem_clock,
            "coherence_protocol" : protocol,
            "entry_cache_size" : 32768,
            "debug" : mh_debug,
            "debug_level" : mh_debug_level,
            "interleave_size" : "64B",    # Interleave at line granularity between memories
            "interleave_step" : str(numMemories * 64) + "B",
            "addr_range_start" : memID*64,
            "addr_range_end" :  1024*1024*1024 - ((numMemories - memID) * 64) + 63,
        })

        # node directory controller port to cpu
        dirNIC = dirctrl.setSubComponent("cpulink", "memHierarchy.MemNICFour")
        dirNIC.addParams({
            "group" : 2,
            "debug" : mh_debug,
            "debug_level" : mh_debug_level,
        })

        dcdata = dirNIC.setSubComponent("data", "kingsley.linkcontrol")
        dcreq = dirNIC.setSubComponent("req", "kingsley.linkcontrol")
        dcfwd = dirNIC.setSubComponent("fwd", "kingsley.linkcontrol")
        dcack = dirNIC.setSubComponent("ack", "kingsley.linkcontrol")
        dcdata.addParams(data_network_params)
        dcreq.addParams(ctrl_network_params)
        dcfwd.addParams(ctrl_network_params)
        dcack.addParams(ctrl_network_params)

        return (dcreq, "rtr_port", mesh_link_latency), (dcack, "rtr_port", mesh_link_latency), \
               (dcfwd, "rtr_port", mesh_link_latency), (dcdata, "rtr_port", mesh_link_latency)

# MMU -> ostlb
# don't need when using pass through TLB
#link_mmu_ostlb_link = sst.Link("link_mmu_ostlb_link")
#link_mmu_ostlb_link.connect( (node_os_mmu, "ostlb", "1ns"), (ostlb, "mmu", "1ns") )

# ostlb -> os l1 cache
link_os_cache_link = sst.Link("link_os_cache_link")
#link_os_cache_link.connect( (ostlbWrapper, "cache_if", "1ns"), (os_cache_2_cpu, "port", "1ns") )
link_os_cache_link.connect( (node_os_mem_if, "port", "1ns"), (os_cache_2_cpu, "port", "1ns") )
link_os_cache_link.setNoCut()

def is_on_top_row(index):
    size = mesh_stops_y * mesh_stops_x
    y = index // size
    return y == 0

def is_on_bottom_row(index):
    size = mesh_stops_y * mesh_stops_x
    y = index // size
    return y == (size - 1)

def is_on_rightmost_column(index):
    size = mesh_stops_y * mesh_stops_x
    max_x = size - 1
    x = index % size
    return x == max_x

def is_on_leftmost_column(index):
    size = mesh_stops_y * mesh_stops_x
    x = index % size
    return x == 0

cpuBuilder = CPU_Builder()
memBuilder = MemBuilder(8192 * 1024 * 1024 * 1024)
dcBuilder  = DCBuilder()
class NodeBuilder:
    def __init__(self):
        self.cpu_count = 0
        self.mem_count = 0

    def build_endpoint(self, nodeId, rtrreq, rtrack, rtrfwd, rtrdata):

        if( nodeId == 0 ):
            if pythonDebug:
                print(f"Node {nodeId} is a OS")
            # OS
            reqport2 = sst.Link("krtr_req_port2_" + str(nodeId))
            reqport2.connect( (rtrreq, "local2", mesh_link_latency), (os_req, "rtr_port", mesh_link_latency) )
            ackport2 = sst.Link("krtr_ack_port2_" + str(nodeId))
            ackport2.connect( (rtrack, "local2", mesh_link_latency), (os_ack, "rtr_port", mesh_link_latency) )
            fwdport2 = sst.Link("krtr_fwd_port2_" + str(nodeId))
            fwdport2.connect( (rtrfwd, "local2", mesh_link_latency), (os_fwd, "rtr_port", mesh_link_latency) )
            dataport2 = sst.Link("kRtr_data_port2_" + str(nodeId))
            dataport2.connect( (rtrdata, "local2", mesh_link_latency), (os_data, "rtr_port", mesh_link_latency) )

        if nodeId in cpu_node_list:
            prefix="node" + str(nodeId) + ".self.cpu_count" + str(self.cpu_count)
            if pythonDebug:
                print(f"Node {nodeId} is a CPU")
                print(f"{prefix}")
            os_hdlr, dtlb, itlb, tilereq, tileack, tilefwd, tiledata = cpuBuilder.build(prefix, nodeId, self.cpu_count)
            # MMU -> dtlb
            link_mmu_dtlb_link = sst.Link(prefix + ".link_mmu_dtlb_link")
            link_mmu_dtlb_link.connect( (node_os_mmu, "core"+ str(self.cpu_count) +".dtlb", "1ns"), dtlb )

            # MMU -> itlb
            link_mmu_itlb_link = sst.Link(prefix + ".link_mmu_itlb_link")
            link_mmu_itlb_link.connect( (node_os_mmu, "core"+ str(self.cpu_count) +".itlb", "1ns"), itlb )

            # CPU os handler -> node OS
            link_core_os_link = sst.Link(prefix + ".link_core_os_link")
            link_core_os_link.connect( os_hdlr, (node_os, "core" + str(self.cpu_count), "5ns") )

            # connect self.cpu_count L2 to mesh
            reqport0 = sst.Link("krtr_req_port0_" + str(nodeId))
            reqport0.connect( (rtrreq, "local0", mesh_link_latency), tilereq )
            ackport0 = sst.Link("krtr_ack_port0_" + str(nodeId))
            ackport0.connect( (rtrack, "local0", mesh_link_latency), tileack )
            fwdport0 = sst.Link("krtr_fwd_port0_" + str(nodeId))
            fwdport0.connect( (rtrfwd, "local0", mesh_link_latency), tilefwd )
            dataport0 = sst.Link("kRtr_data_port0_" + str(nodeId))
            dataport0.connect( (rtrdata, "local0", mesh_link_latency), tiledata )

            self.cpu_count += 1

        elif nodeId in mem_node_list:
            if pythonDebug:
                print(f"Node {nodeId} is a MEM")
            req, ack, fwd, data = memBuilder.build(nodeId, self.mem_count)

            if( is_on_top_row(nodeId) ):
                port = "north"
            elif( is_on_rightmost_column(nodeId) ):
                port = "east"
            elif( is_on_leftmost_column(nodeId) ):
                port = "west"
            elif( is_on_bottom_row(nodeId) ):
                port = "south"

            rtrreqport = sst.Link("krtr_req_" + port + "_" +str(nodeId))
            rtrreqport.connect( (rtrreq, port, mesh_link_latency), req )
            rtrackport = sst.Link("krtr_ack_" + port + "_" + str(nodeId))
            rtrackport.connect( (rtrack, port, mesh_link_latency), ack )
            rtrfwdport = sst.Link("krtr_fwd_" + port + "_" + str(nodeId))
            rtrfwdport.connect( (rtrfwd, port, mesh_link_latency), fwd )
            rtrdataport = sst.Link("kRtr_data_" + port + "_" + str(nodeId))
            rtrdataport.connect( (rtrdata, port, mesh_link_latency), data )

            ## Build DCs
            req, ack, fwd, data = dcBuilder.build(nodeId, self.mem_count)
            reqport1 = sst.Link("krtr_req_port1_" + str(nodeId))
            reqport1.connect( (rtrreq, "local1", mesh_link_latency), req )
            ackport1 = sst.Link("krtr_ack_port1_" + str(nodeId))
            ackport1.connect( (rtrack, "local1", mesh_link_latency), ack )
            fwdport1 = sst.Link("krtr_fwd_port1_" + str(nodeId))
            fwdport1.connect( (rtrfwd, "local1", mesh_link_latency), fwd )
            dataport1 = sst.Link("kRtr_data_port1_" + str(nodeId))
            dataport1.connect( (rtrdata, "local1", mesh_link_latency), data )

            self.mem_count += 1

        else:
            if pythonDebug:
                print("Empty node")

node_builder = NodeBuilder();
def build_mesh():
    # Build Kingsley Mesh
    kRtrReq=[]
    kRtrAck=[]
    kRtrFwd=[]
    kRtrData=[]
    for y in range(0, mesh_stops_y):
        for x in range (0, mesh_stops_x):
            nodeNum = len(kRtrReq)
            kRtrReq.append(sst.Component("krtr_req_" + str(nodeNum), "kingsley.noc_mesh"))
            kRtrReq[-1].addParams(ctrl_network_params)
            kRtrAck.append(sst.Component("krtr_ack_" + str(nodeNum), "kingsley.noc_mesh"))
            kRtrAck[-1].addParams(ctrl_network_params)
            kRtrFwd.append(sst.Component("krtr_fwd_" + str(nodeNum), "kingsley.noc_mesh"))
            kRtrFwd[-1].addParams(ctrl_network_params)
            kRtrData.append(sst.Component("krtr_data_" + str(nodeNum), "kingsley.noc_mesh"))
            kRtrData[-1].addParams(data_network_params)

            kRtrReq[-1].addParams({"local_ports" : 3})
            kRtrAck[-1].addParams({"local_ports" : 3})
            kRtrFwd[-1].addParams({"local_ports" : 3})
            kRtrData[-1].addParams({"local_ports" : 3})

    i = 0
    for y in range(0, mesh_stops_y):
        for x in range (0, mesh_stops_x):

            ## North-south connections
            if y != (mesh_stops_y -1):
                kRtrReqNS = sst.Link("krtr_req_ns_" + str(i))
                kRtrReqNS.connect( (kRtrReq[i], "south", mesh_link_latency), (kRtrReq[i + mesh_stops_x], "north", mesh_link_latency) )
                kRtrAckNS = sst.Link("krtr_ack_ns_" + str(i))
                kRtrAckNS.connect( (kRtrAck[i], "south", mesh_link_latency), (kRtrAck[i + mesh_stops_x], "north", mesh_link_latency) )
                kRtrFwdNS = sst.Link("krtr_fwd_ns_" + str(i))
                kRtrFwdNS.connect( (kRtrFwd[i], "south", mesh_link_latency), (kRtrFwd[i + mesh_stops_x], "north", mesh_link_latency) )
                kRtrDataNS = sst.Link("krtr_data_ns_" + str(i))
                kRtrDataNS.connect( (kRtrData[i], "south", mesh_link_latency), (kRtrData[i + mesh_stops_x], "north", mesh_link_latency) )

            ## East-West connections
            if x != (mesh_stops_x - 1):
                kRtrReqEW = sst.Link("krtr_req_ew_" + str(i))
                kRtrReqEW.connect( (kRtrReq[i], "east", mesh_link_latency), (kRtrReq[i+1], "west", mesh_link_latency) )
                kRtrAckEW = sst.Link("krtr_ack_ew_" + str(i))
                kRtrAckEW.connect( (kRtrAck[i], "east", mesh_link_latency), (kRtrAck[i+1], "west", mesh_link_latency) )
                kRtrFwdEW = sst.Link("krtr_fwd_ew_" + str(i))
                kRtrFwdEW.connect( (kRtrFwd[i], "east", mesh_link_latency), (kRtrFwd[i+1], "west", mesh_link_latency) )
                kRtrDataEW = sst.Link("krtr_data_ew_" + str(i))
                kRtrDataEW.connect( (kRtrData[i], "east", mesh_link_latency), (kRtrData[i+1], "west", mesh_link_latency) )

            node_builder.build_endpoint(i, kRtrReq[i], kRtrAck[i], kRtrFwd[i], kRtrData[i])
            i = i + 1

## main-ish
build_mesh()

##Enable statistics
sst.setStatisticLoadLevel(7)
#sst.setStatisticOutput("sst.statOutputCSV")
sst.enableAllStatisticsForAllComponents()


