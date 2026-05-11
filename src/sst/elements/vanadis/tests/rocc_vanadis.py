import os
import sst
import argparse

############### Testing for ROCC interface in Vanadis ####################
#
## Env Variables
#   Variable                     | Default                                          | Description/Options
#  ------------------------------|--------------------------------------------------|---------------------------
#   VANADIS_EXE                  | ./small/basic-io/hello-world/<ISA>/hello-world   | Which application to run
#   VANADIS_EXE_ARGS             | ""                                               | Any arguments for the application
#   VANADIS_ISA                  | RISCV64                                          | Which ISA to use (MIPS or RISCV64)
#   VANADIS_LOADER_MODE          | 0                                                | Instruction uOp cache configuration. 0=LRU, 1=infinite
#   VANADIS_VERBOSE              | 0                                                | Verbosity level for core components.
#   VANADIS_OS_VERBOSE           | 0                                                | Verbosity level for the OS component. Defaults to 'VANADIS_VERBOSE' if not set.
#   VANADIS_PIPE_TRACE           | ""                                               | Filename to send a trace to or "" for none
#   VANADIS_LSQ_LD_ENTRIES       | 16                                               | Number of load entries in the load-store queue
#   VANADIS_LSQ_ST_ENTRIES       | 8                                                | Number of store entries in the load-store queue
#   VANADIS_ROB_SLOTS            | 64                                               | Number of entries in the reorder buffer
#   VANADIS_RETIRES_PER_CYCLE    | 4                                                | Maximum number of instructions to retire per cycle
#   VANADIS_ISSUES_PER_CYCLE     | 4                                                | Maximum number of instructions to issue per cycle
#   VANADIS_DECODES_PER_CYCLE    | 4                                                | Maximum number of instructions to decode per cycle
#   VANADIS_INTEGER_ARITH_CYCLES | 2                                                | Cycles per integer operation
#   VANADIS_INTEGER_ARITH_UNITS  | 2                                                | Number of integer ALUs
#   VANADIS_FP_ARITH_CYCLES      | 8                                                | Cycles per floating point operation
#   VANADIS_FP_ARITH_UNITS       | 2                                                | Number of floating point units
#   VANADIS_BRANCH_ARITH_CYCLES  | 2                                                | Cycles per branch operation
#   VANADIS_CPU_CLOCK            | 2.3GHz                                           | Core clock frequency
#   VANADIS_NUM_CORES            | 1                                                | Number of cores
#   VANADIS_NUM_HW_THREADS       | 1                                                | Number of hardware threads per core
#   VANADIS_LIBRARY              | vanadis                                          | Which vanadis library to use (vanadis or vanadisdbg)
#   VANADIS_HALT_AT_ADDRESS      | 0                                                | If not 0, an instruction address to halt at
#   VANADIS_TLB_IFACE            | 0                                                | Whether to put TLBs in the core's memory interface (1) or place them between the interface and L1 (0)
#
#

# Parse arguments. If not found, defer to environment variables.
parser = argparse.ArgumentParser()
parser.add_argument("-e", "--exe", help="Executable to run on vanadis. Default is './small/basic-io/hello-world/<ISA>/hello-world'")
parser.add_argument("-a", "--exe_args", help="Arguments for the executable in '--exe'. Default is ''.")
parser.add_argument("-i", "--isa", help="ISA to simulation, 'mipsel' or 'riscv64' (default)")
parser.add_argument("-v", "--verbose", help="Verbosity level for vanadis core components and subcomponents. Default is 0.")
parser.add_argument("-o", "--os-verbose", help="Verbosity level for the OS component. Defaults to --verbose if not set.")
parser.add_argument("-c", "--num-cores", help="Number of cores. Default is 1.")
parser.add_argument("-t", "--num-hw-threads", help="Number of hardware threads per core. Default is 1.")
parser.add_argument("--loader-mode", help="Instruction micro-op cache setting '0'=LRU (default) or '1'=infinite")
parser.add_argument("--pipe-trace", help="Filename to send a trace to or "" for none (default).")
parser.add_argument("--lsq-ld-entries", help="Number of load entries in the load-store queue. Default is 16.")
parser.add_argument("--lsq-st-entries", help="Number of store entries in the load-store queue. Default is 8.")
parser.add_argument("--rob-entries", help="Number of entries per hw thread in the reorder buffer. Default is 64.")
parser.add_argument("--retires-per-cycle", help="Maximum number of instructions to retire per cycle. Default is 4.")
parser.add_argument("--issues-per-cycle", help="Maximum number of instructions to issue per cycle. Default is 4.")
parser.add_argument("--decodes-per-cycle", help="Maximum number of instructions to decode per cycle. Default is 4.")
parser.add_argument("--int-arith-cycles", help="Cycles per integer operation. Default is 2.")
parser.add_argument("--int-arith-units", help="Number of integer ALUs. Default is 2.")
parser.add_argument("--fp-arith-cycles", help="Cycles per floating point operation. Default is 8.")
parser.add_argument("--fp-arith-units", help="Number of floating point units. Default is 2.")
parser.add_argument("--branch-arith-cycles", help="Cycles per branch operation. Default is 2.")
parser.add_argument("--cpu-clock", help="Core clock frequency. Default is 2.3GHz.")
parser.add_argument("--library", help="Which vanadis library to use, 'vanadis' or 'vanadisdbg'. Default is vanadis.")
parser.add_argument("--halt-at-address", help="An optional instruction address at which to end simulation. 0 indicates none (default).")
parser.add_argument("--tlb-iface", help="Whether to put TLBs in the core's memory interface (1) or place them between the interface and L1 (0, default).")
args = parser.parse_args()


# Defaults needed in parsing
testDir="basic-io"
exe = "hello-world"

# Popoulate argument vars from parser and/or environment variables
vanadis_isa = os.getenv("VANADIS_ISA", "riscv64") if args.isa == None else args.isa
if vanadis_isa in ["rv", "riscv", "riscv64", "RISCV64", "RV", "RISCV"]:
    vanadis_isa = "RISCV64"
    isa="riscv64"
elif vanadis_isa in ["mips", "mipsel", "MIPS"]:
    vanadis_isa = "MIPS"
    isa = "mipsel"

full_exe_name = os.getenv("VANADIS_EXE", "./small/" + testDir + "/" + exe +  "/" + isa + "/" + exe ) if args.exe == None else args.exe
app_args = os.getenv("VANADIS_EXE_ARGS", "") if args.exe_args == None else args.exe_args
verbosity = int(os.getenv("VANADIS_VERBOSE",0)) if args.verbose == None else int(args.verbose)
os_verbosity = int(os.getenv("VANADIS_OS_VERBOSE", verbosity)) if args.os_verbose == None else int(args.os_verbose)
pipe_trace_file = os.getenv("VANADIS_PIPE_TRACE", "") if args.pipe_trace == None else args.pipe_trace
lsq_ld_entries = os.getenv("VANADIS_LSQ_LD_ENTRIES", "16") if args.lsq_ld_entries == None else args.lsq_ld_entries
lsq_st_entries = os.getenv("VANADIS_LSQ_ST_ENTRIES", "8") if args.lsq_st_entries == None else args.lsq_st_entries
rob_slots = os.getenv("VANADIS_ROB_SLOTS", 64) if args.rob_entries == None else args.rob_entries
retires_per_cycle = os.getenv("VANADIS_RETIRES_PER_CYCLE", 4) if args.retires_per_cycle == None else args.retires_per_cycle
issues_per_cycle = os.getenv("VANADIS_ISSUES_PER_CYCLE", 4) if args.issues_per_cycle == None else args.issues_per_cycle
decodes_per_cycle = os.getenv("VANADIS_DECODES_PER_CYCLE", 4) if args.decodes_per_cycle == None else args.decodes_per_cycle
integer_arith_cycles = int(os.getenv("VANADIS_INTEGER_ARITH_CYCLES", 2)) if args.int_arith_cycles == None else int(args.int_arith_cycles)
integer_arith_units = int(os.getenv("VANADIS_INTEGER_ARITH_UNITS", 2)) if args.int_arith_units == None else int(args.int_arith_units)
fp_arith_cycles = int(os.getenv("VANADIS_FP_ARITH_CYCLES", 8)) if args.fp_arith_cycles == None else int(args.fp_arith_cycles)
fp_arith_units = int(os.getenv("VANADIS_FP_ARITH_UNITS", 2)) if args.fp_arith_units == None else int(args.fp_aright_units)
branch_arith_cycles = int(os.getenv("VANADIS_BRANCH_ARITH_CYCLES", 2)) if args.branch_arith_cycles == None else int(args.branch_arith_cycles)
env_tlb_config = int(os.getenv("VANADIS_TLB_IFACE", "0")) if args.tlb_iface == None else int(args.tlb_iface)
lib = os.getenv("VANADIS_LIBRARY", "vanadis") if args.library == None else args.library
loader_mode = os.getenv("VANADIS_LOADER_MODE", 0) if args.loader_mode == None else args.loader_mode
cpu_clock = os.getenv("VANADIS_CPU_CLOCK", "2.3GHz") if args.cpu_clock == None else args.cpu_clock
num_cpus = int(os.getenv("VANADIS_NUM_CORES", 1)) if args.num_cores == None else int(args.num_cores)
num_threads = int(os.getenv("VANADIS_NUM_HW_THREADS", 1)) if args.num_hw_threads == None else int(args.num_hw_threads)
halt_address = os.getenv("VANADIS_HALT_AT_ADDRESS", 0) if args.halt_at_address == None else args.halt_at_address

# For convenience later
exe_name= full_exe_name.split("/")[-1]


mh_debug_level=10
mh_debug=0
# this has to be a string
dbgAddr="0"
stopDbg="0"

checkpointDir = ""
checkpoint = ""

#checkpointDir = "checkpoint0"
#checkpoint = "load"
#checkpoint = "save"

pythonDebug=False

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

# Define SST core options
sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stop-at", "0 ns")

# Tell SST what statistics handling we want
sst.setStatisticLoadLevel(4)
sst.setStatisticOutput("sst.statOutputConsole")

vanadis_cpu_type = lib + ".core"

if (verbosity > 0):
    print("Verbosity: " + str(verbosity) + " -> loading Vanadis CPU type: " + vanadis_cpu_type)

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

vanadis_decoder = lib + ".Vanadis" + vanadis_isa + "Decoder"
vanadis_os_hdlr = lib + ".Vanadis" + vanadis_isa + "OSHandler"


protocol="MESI"

# OS related params
osParams = {
    "processDebugLevel" : 0,
    "dbgLevel" : os_verbosity,
    "dbgMask" : 8,
    "cores" : num_cpus,
    "hardwareThreadCount" : num_threads,
    "page_size"  : 4096,
    "physMemSize" : physMemSize,
    "useMMU" : True,
    "checkpointDir" : checkpointDir,
    "checkpoint" : checkpoint
}

processList = (
    ( 1, {
        "env_count" : 1,
        "env0" : "OMP_NUM_THREADS={}".format(num_cpus*num_threads),
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
    "num_cores": num_cpus,
    "num_threads": num_threads,
    "page_size": 4096,
}

memRtrParams ={
      "xbar_bw" : "1GB/s",
      "link_bw" : "1GB/s",
      "input_buf_size" : "2KB",
      "num_ports" : str(num_cpus+2),
      "flit_size" : "72B",
      "output_buf_size" : "2KB",
      "id" : "0",
      "topology" : "merlin.singlerouter"
}

dirCtrlParams = {
      "coherence_protocol" : protocol,
      "entry_cache_size" : "1024",
      "debug" : mh_debug,
      "debug_level" : mh_debug_level,
      "addr_range_start" : "0x0",
      "addr_range_end" : "0xFFFFFFFF"
}

dirNicParams = {
      "network_bw" : "25GB/s",
      "group" : 2,
}

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

memParams = {
      "mem_size" : "4GiB",
      "access_time" : "1 ns"
}

# CPU related params
tlbParams = {
    "debug_level": 0,
    "hit_latency": 1,
    "num_hardware_threads": num_threads,
    "num_tlb_entries_per_thread": 64,
    "tlb_set_size": 4,
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

cpuParams = {
    "clock" : cpu_clock,
    "verbose" : verbosity,
    "hardware_threads": num_threads,
    "physical_fp_registers" : 168 * num_threads,
    "physical_integer_registers" : 180 * num_threads,
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
    "pause_when_retire_address" : halt_address,
    "start_verbose_when_issue_address": dbgAddr,
    "stop_verbose_when_retire_address": stopDbg,
    "print_rob" : False,
    "checkpointDir" : checkpointDir,
    "checkpoint" : checkpoint
}

lsqParams = {
    "verbose" : verbosity,
    "address_mask" : 0xFFFFFFFF,
    "max_stores" : lsq_st_entries,
    "max_loads" : lsq_ld_entries,
}

roccParams = {
    "clock" : cpu_clock,
    "verbose" : verbosity,
    "max_instructions" : 8
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
    "associativity" : "16",
    "cache_line_size" : "64",
    "cache_size" : "1MB",
    "mshr_latency_cycles": 3,
    "debug" : mh_debug,
    "debug_level" : mh_debug_level,
}
busParams = {
    "bus_frequency" : cpu_clock,
}

l2memLinkParams = {
    "group" : 1,
    "network_bw" : "25GB/s"
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
        for n in range(num_threads):
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

        cpu_rocc0 = cpu.setSubComponent( "rocc", "vanadis.VanadisRoCCBasic", 0)
        cpu_rocc0.addParams(roccParams)
        cpu_rocc0.enableAllStatistics()

        cpu_rocc1 = cpu.setSubComponent( "rocc", "vanadis.VanadisRoCCBasic", 1)
        cpu_rocc1.addParams(roccParams)
        cpu_rocc1.enableAllStatistics()

        # CPU.lsq mem interface which connects to D-cache
        cpuDcacheIf = cpu_lsq.setSubComponent( "memory_interface", "memHierarchy.standardInterface" )

        # CPU.rocc mem interface which connects to D-cache
        rocc0DcacheIf = cpu_rocc0.setSubComponent( "memory_interface", "memHierarchy.standardInterface" )

        # CPU.rocc mem interface which connects to D-cache
        rocc1DcacheIf = cpu_rocc1.setSubComponent( "memory_interface", "memHierarchy.standardInterface" )

        # CPU.mem interface for I-cache
        cpuIcacheIf = cpu.setSubComponent( "mem_interface_inst", "memHierarchy.standardInterface" )

        # L1 D-cache
        cpu_l1dcache = sst.Component(prefix + ".l1dcache", "memHierarchy.Cache")
        cpu_l1dcache.addParams( l1dcacheParams )

        # L1 I-cache to cpu interface
        l1dcache_2_cpu     = cpu_l1dcache.setSubComponent("highlink", "memHierarchy.MemLink")
        # L1 I-cache to L2 interface
        l1dcache_2_l2cache = cpu_l1dcache.setSubComponent("lowlink", "memHierarchy.MemLink")

        # L2 I-cache
        cpu_l1icache = sst.Component( prefix + ".l1icache", "memHierarchy.Cache")
        cpu_l1icache.addParams( l1icacheParams )

        # L1 I-iache to cpu interface
        l1icache_2_cpu     = cpu_l1icache.setSubComponent("highlink", "memHierarchy.MemLink")
        # L1 I-cache to L2 interface
        l1icache_2_l2cache = cpu_l1icache.setSubComponent("lowlink", "memHierarchy.MemLink")

        # L2 cache
        cpu_l2cache = sst.Component(prefix+".l2cache", "memHierarchy.Cache")
        cpu_l2cache.addParams( l2cacheParams )

        # L2 cache cpu interface
        l2cache_2_l1caches = cpu_l2cache.setSubComponent("highlink", "memHierarchy.MemLink")

        # L2 cache mem interface
        l2cache_2_mem = cpu_l2cache.setSubComponent("lowlink", "memHierarchy.MemNIC")
        l2cache_2_mem.addParams( l2memLinkParams )

        # L1 to L2 buss
        cache_bus = sst.Component(prefix+".bus", "memHierarchy.Bus")
        cache_bus.addParams(busParams)

        # Processors (Vanadis and accelerator(s)) to L1 bus
        processor_bus = sst.Component("processorBus", "memHierarchy.Bus")
        processor_bus.addParams(busParams)

        # CPU data TLB
        dtlbWrapper = sst.Component(prefix+".dtlb", "mmu.tlb_wrapper")
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

        # CPU (data) -> processor_bus
        link_lsq_l1dcache_link = sst.Link(prefix+".link_cpu_dbus_link")
        link_lsq_l1dcache_link.connect( (cpuDcacheIf, "lowlink", "1ns"), (processor_bus, "highlink0", "1ns") )
        link_lsq_l1dcache_link.setNoCut()

        # RoCC (data) -> processor_bus
        link_rocc0_l1dcache_link = sst.Link(prefix+".link_rocc0_dbus_link")

        link_rocc0_l1dcache_link.connect( (rocc0DcacheIf, "lowlink", "1ns"),
                                        (processor_bus, "highlink1", "1ns"))
        link_rocc0_l1dcache_link.setNoCut()

        # RoCC (data) -> processor_bus
        link_rocc1_l1dcache_link = sst.Link(prefix+".link_rocc1_dbus_link")

        link_rocc1_l1dcache_link.connect( (rocc1DcacheIf, "lowlink", "1ns"),
                                        (processor_bus, "highlink2", "1ns"))
        link_rocc1_l1dcache_link.setNoCut()

        # processor_bus -> L1 cache
        link_bus_l1cache_link = sst.Link(prefix+".link_bus_l1cache_link")
        link_bus_l1cache_link.connect( (processor_bus, "lowlink0", "1ns"), (dtlbWrapper, "highlink", "1ns") )
        link_bus_l1cache_link.setNoCut()

        # data TLB -> data L1
        link_cpu_l1dcache_link = sst.Link(prefix+".link_cpu_l1dcache_link")
        link_cpu_l1dcache_link.connect( (dtlbWrapper, "lowlink", "1ns"), (l1dcache_2_cpu, "port", "1ns") )
        link_cpu_l1dcache_link.setNoCut()

        # CPU (instruction) -> TLB -> Cache
        link_cpu_itlb_link = sst.Link(prefix+".link_cpu_itlb_link")
        link_cpu_itlb_link.connect( (cpuIcacheIf, "lowlink", "1ns"), (itlbWrapper, "highlink", "1ns") )
        link_cpu_itlb_link.setNoCut()

        # instruction TLB -> instruction L1
        link_cpu_l1icache_link = sst.Link(prefix+".link_cpu_l1icache_link")
        link_cpu_l1icache_link.connect( (itlbWrapper, "lowlink", "1ns"), (l1icache_2_cpu, "port", "1ns") )
        link_cpu_l1icache_link.setNoCut()

        # data L1 -> bus
        link_l1dcache_l2cache_link = sst.Link(prefix+".link_l1dcache_l2cache_link")
        link_l1dcache_l2cache_link.connect( (l1dcache_2_l2cache, "port", "1ns"), (cache_bus, "highlink0", "1ns") )
        link_l1dcache_l2cache_link.setNoCut()

        # instruction L1 -> bus
        link_l1icache_l2cache_link = sst.Link(prefix+".link_l1icache_l2cache_link")
        link_l1icache_l2cache_link.connect( (l1icache_2_l2cache, "port", "1ns"), (cache_bus, "highlink1", "1ns") )
        link_l1icache_l2cache_link.setNoCut()

        # BUS to L2 cache
        link_bus_l2cache_link = sst.Link(prefix+".link_bus_l2cache_link")
        link_bus_l2cache_link.connect( (cache_bus, "lowlink0", "1ns"), (l2cache_2_l1caches, "port", "1ns") )
        link_bus_l2cache_link.setNoCut()

        return (cpu, "os_link", "5ns"), (l2cache_2_mem, "port", "1ns") , (dtlb, "mmu", "1ns"), (itlb, "mmu", "1ns")


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
os_cache_2_cpu = os_cache.setSubComponent("highlink", "memHierarchy.MemLink")
os_cache_2_mem = os_cache.setSubComponent("lowlink", "memHierarchy.MemNIC")
os_cache_2_mem.addParams( l2memLinkParams )

# node memory router
comp_chiprtr = sst.Component("chiprtr", "merlin.hr_router")
comp_chiprtr.addParams(memRtrParams)
comp_chiprtr.setSubComponent("topology","merlin.singlerouter")

# node directory controller
dirctrl = sst.Component("dirctrl", "memHierarchy.DirectoryController")
dirctrl.addParams(dirCtrlParams)

# node directory controller port to memory
dirtoM = dirctrl.setSubComponent("lowlink", "memHierarchy.MemLink")
# node directory controller port to cpu
dirNIC = dirctrl.setSubComponent("highlink", "memHierarchy.MemNIC")
dirNIC.addParams(dirNicParams)

# node memory controller
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams( memCtrlParams )

# node memory controller port to directory controller
memToDir = memctrl.setSubComponent("highlink", "memHierarchy.MemLink")

# node memory controller backend
memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams(memParams)

# node OS data TLB
#ostlbWrapper = sst.Component("ostlb", "mmu.tlb_wrapper")
#ostlbWrapper.addParams(tlbWrapperParams)
#ostlb = ostlbWrapper.setSubComponent("tlb", "mmu." + tlbType );
#ostlb = ostlbWrapper.setSubComponent("tlb", "mmu.passThroughTLB" );
#ostlb.addParams(tlbParams)

# OS (data) -> TLB -> Cache
#link_os_ostlb_link = sst.Link("link_os_ostlb_link")
#link_os_ostlb_link.connect( (node_os_mem_if, "port", "1ns"), (ostlbWrapper, "cpu_if", "1ns") )

# Directory controller to memory router
link_dir_2_rtr = sst.Link("link_dir_2_rtr")
link_dir_2_rtr.connect( (comp_chiprtr, "port"+str(num_cpus), "1ns"), (dirNIC, "port", "1ns") )
link_dir_2_rtr.setNoCut()

# Directory controller to memory controller
link_dir_2_mem = sst.Link("link_dir_2_mem")
link_dir_2_mem.connect( (dirtoM, "port", "1ns"), (memToDir, "port", "1ns") )
link_dir_2_mem.setNoCut()

# MMU -> ostlb
# don't need when using pass through TLB
#link_mmu_ostlb_link = sst.Link("link_mmu_ostlb_link")
#link_mmu_ostlb_link.connect( (node_os_mmu, "ostlb", "1ns"), (ostlb, "mmu", "1ns") )

# ostlb -> os l1 cache
link_os_cache_link = sst.Link("link_os_cache_link")
#link_os_cache_link.connect( (ostlbWrapper, "cache_if", "1ns"), (os_cache_2_cpu, "port", "1ns") )
link_os_cache_link.connect( (node_os_mem_if, "lowlink", "1ns"), (os_cache_2_cpu, "port", "1ns") )
link_os_cache_link.setNoCut()

os_cache_2_rtr = sst.Link("os_cache_2_rtr")
os_cache_2_rtr.connect( (os_cache_2_mem, "port", "1ns"), (comp_chiprtr, "port"+str(num_cpus+1), "1ns") )
os_cache_2_rtr.setNoCut()

cpuBuilder = CPU_Builder()

# build all CPUs
nodeId = 0
for cpu in range(num_cpus):

    prefix="node" + str(nodeId) + ".cpu" + str(cpu)
    os_hdlr, l2cache, dtlb, itlb = cpuBuilder.build(prefix, nodeId, cpu)

    # MMU -> dtlb
    link_mmu_dtlb_link = sst.Link(prefix + ".link_mmu_dtlb_link")
    link_mmu_dtlb_link.connect( (node_os_mmu, "core"+ str(cpu) +".dtlb", "1ns"), dtlb )

    # MMU -> itlb
    link_mmu_itlb_link = sst.Link(prefix + ".link_mmu_itlb_link")
    link_mmu_itlb_link.connect( (node_os_mmu, "core"+ str(cpu) +".itlb", "1ns"), itlb )

    # CPU os handler -> node OS
    link_core_os_link = sst.Link(prefix + ".link_core_os_link")
    link_core_os_link.connect( os_hdlr, (node_os, "core" + str(cpu), "5ns") )

    # connect cpu L2 to router
    link_l2cache_2_rtr = sst.Link(prefix + ".link_l2cache_2_rtr")
    link_l2cache_2_rtr.connect( l2cache, (comp_chiprtr, "port" + str(cpu), "1ns") )
