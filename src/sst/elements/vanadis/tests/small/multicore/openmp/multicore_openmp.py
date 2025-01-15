import os
import sst

mh_debug_level = 10
mh_debug = 0
dbgAddr = 0
stopDbg = 0

pythonDebug = False

# Hard-code ISA to RISC-V
vanadis_isa = "RISCV64"
isa = "riscv64"

# Loader mode
loader_mode = "0"

# Name of the binary to execute
exe = "openmp_example"
full_exe_name = "./" + exe
exe_name = exe

physMemSize = "4GiB"

tlbType = "simpleTLB"
mmuType = "simpleMMU"

# Define SST core options
sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stop-at", "0 ns")

# Statistics
sst.setStatisticLoadLevel(4)
sst.setStatisticOutput("sst.statOutputConsole")

verbosity = 0
os_verbosity = 0
pipe_trace_file = ""
lsq_entries = 32

rob_slots = 64
retires_per_cycle = 4
issues_per_cycle = 4
decodes_per_cycle = 4

integer_arith_cycles = 2
integer_arith_units = 2
fp_arith_cycles = 8
fp_arith_units = 2
branch_arith_cycles = 2

auto_clock_sys = "no"
cpu_clock = "2.3GHz"

# Force exactly 2 cores (numCpus=2), each with 1 thread (numThreads=1)
numCpus = 2
numThreads = 1

vanadis_cpu_type = "vanadis.dbg_VanadisCPU"

# (Optional) Command-line arguments logic omitted in this example
app_args = ""

vanadis_decoder = "vanadis.Vanadis" + vanadis_isa + "Decoder"
vanadis_os_hdlr = "vanadis.Vanadis" + vanadis_isa + "OSHandler"

protocol = "MESI"

# OS params
osParams = {
    "processDebugLevel": 0,
    "dbgLevel": os_verbosity,
    "dbgMask": 0xFFFFFFFF,
    "cores": numCpus,
    "hardwareThreadCount": numThreads,
    "heap_start": 512 * 1024 * 1024,
    "heap_end": (2 * 1024 * 1024 * 1024) - 4096,
    "page_size": 4096,
    "heap_verbose": verbosity,
    "physMemSize": physMemSize,
    "useMMU": True,
}

# Process list: run openmp_example with OMP_NUM_THREADS = 2 * 1 = 2
processList = (
    (
        1,
        {
            "env_count": 1,
            "env0": "OMP_NUM_THREADS={}".format(numCpus * numThreads),
            "exe": full_exe_name,
            "arg0": exe_name,
        },
    ),
)

osl1cacheParams = {
    "access_latency_cycles": "2",
    "cache_frequency": cpu_clock,
    "replacement_policy": "lru",
    "coherence_protocol": protocol,
    "associativity": "8",
    "cache_line_size": "64",
    "cache_size": "32 KB",
    "L1": "1",
    "debug": mh_debug,
    "debug_level": mh_debug_level,
}

mmuParams = {
    "debug_level": 0,
    "num_cores": numCpus,
    "num_threads": numThreads,
    "page_size": 4096,
}

memRtrParams = {
    "xbar_bw": "1GB/s",
    "link_bw": "1GB/s",
    "input_buf_size": "2KB",
    "num_ports": str(numCpus + 2),
    "flit_size": "72B",
    "output_buf_size": "2KB",
    "id": "0",
    "topology": "merlin.singlerouter",
}

dirCtrlParams = {
    "coherence_protocol": protocol,
    "entry_cache_size": "1024",
    "debug": mh_debug,
    "debug_level": mh_debug_level,
    "addr_range_start": "0x0",
    "addr_range_end": "0xFFFFFFFF",
}

dirNicParams = {
    "network_bw": "25GB/s",
    "group": 2,
}

memCtrlParams = {
    "clock": cpu_clock,
    "backend.mem_size": physMemSize,
    "backing": "malloc",
    "initBacking": 1,
    "addr_range_start": 0,
    "addr_range_end": 0xFFFFFFFF,
    "debug_level": mh_debug_level,
    "debug": mh_debug,
}

memParams = {
    "mem_size": "4GiB",
    "access_time": "1 ns",
}

tlbParams = {
    "debug_level": 0,
    "hitLatency": 1,
    "num_hardware_threads": numThreads,
    "num_tlb_entries_per_thread": 64,
    "tlb_set_size": 4,
}

tlbWrapperParams = {
    "debug_level": 0,
}

decoderParams = {
    "loader_mode": loader_mode,
    "uop_cache_entries": 1536,
    "predecode_cache_entries": 4,
}

osHdlrParams = {
    "verbose": os_verbosity,
}

branchPredParams = {
    "branch_entries": 32,
}

cpuParams = {
    "clock": cpu_clock,
    "verbose": verbosity,
    "hardware_threads": numThreads,
    "physical_fp_registers": 168 * numThreads,
    "physical_integer_registers": 180 * numThreads,
    "integer_arith_cycles": integer_arith_cycles,
    "integer_arith_units": integer_arith_units,
    "fp_arith_cycles": fp_arith_cycles,
    "fp_arith_units": fp_arith_units,
    "branch_unit_cycles": branch_arith_cycles,
    "print_int_reg": False,
    "print_fp_reg": False,
    "pipeline_trace_file": pipe_trace_file,
    "reorder_slots": rob_slots,
    "decodes_per_cycle": decodes_per_cycle,
    "issues_per_cycle": issues_per_cycle,
    "retires_per_cycle": retires_per_cycle,
    "auto_clock_syscall": auto_clock_sys,
    "pause_when_retire_address": 0,
    "start_verbose_when_issue_address": dbgAddr,
    "stop_verbose_when_retire_address": stopDbg,
    "print_rob": False,
}

lsqParams = {
    "verbose": verbosity,
    "address_mask": 0xFFFFFFFF,
    "load_store_entries": lsq_entries,
    "fault_non_written_loads_after": 0,
    "check_memory_loads": "no",
}

roccParams = {
    "clock": cpu_clock,
    "verbose": 4,
    "max_instructions": 8,
}

arrayParams = {
    "arrayLatency": "100ns",
    "clock": cpu_clock,
    "max_instructions": 8,
    "verbose": 20,
    "mmioAddr": 0,
    "numArrays": 1,
    "arrayInputSize": 3,
    "arrayOutputSize": 16,
}

roccarrayParams = {
    "inputOperandSize": 4,
    "outputOperandSize": 4,
}

roccParams.update(roccarrayParams)
arrayParams.update(roccarrayParams)
roccParams.update(arrayParams)

l1dcacheParams = {
    "access_latency_cycles": "2",
    "cache_frequency": cpu_clock,
    "replacement_policy": "lru",
    "coherence_protocol": protocol,
    "associativity": "8",
    "cache_line_size": "64",
    "cache_size": "32 KB",
    "L1": "1",
    "debug": mh_debug,
    "debug_level": mh_debug_level,
}

l1icacheParams = {
    "access_latency_cycles": "2",
    "cache_frequency": cpu_clock,
    "replacement_policy": "lru",
    "coherence_protocol": protocol,
    "associativity": "8",
    "cache_line_size": "64",
    "cache_size": "32 KB",
    "prefetcher": "cassini.NextBlockPrefetcher",
    "prefetcher.reach": 1,
    "L1": "1",
    "debug": mh_debug,
    "debug_level": mh_debug_level,
}

l2cacheParams = {
    "access_latency_cycles": "14",
    "cache_frequency": cpu_clock,
    "replacement_policy": "lru",
    "coherence_protocol": protocol,
    "associativity": "16",
    "cache_line_size": "64",
    "cache_size": "1MB",
    "mshr_latency_cycles": 3,
    "debug": mh_debug,
    "debug_level": mh_debug_level,
}

busParams = {
    "bus_frequency": cpu_clock,
}

l2memLinkParams = {
    "group": 1,
    "network_bw": "25GB/s",
}


class CPU_Builder:
    def __init__(self):
        pass

    def build(self, prefix, nodeId, cpuId):

        if pythonDebug:
            print("build {}".format(prefix))

        # CPU
        cpu = sst.Component(prefix, vanadis_cpu_type)
        cpu.addParams(cpuParams)
        cpu.addParam("core_id", cpuId)
        cpu.enableAllStatistics()

        # CPU.decoder
        for n in range(numThreads):
            decode = cpu.setSubComponent("decoder" + str(n), vanadis_decoder)
            decode.addParams(decoderParams)
            decode.enableAllStatistics()

            # OS handler
            os_hdlr = decode.setSubComponent("os_handler", vanadis_os_hdlr)
            os_hdlr.addParams(osHdlrParams)

            # Branch predictor
            branch_pred = decode.setSubComponent("branch_unit", "vanadis.VanadisBasicBranchUnit")
            branch_pred.addParams(branchPredParams)
            branch_pred.enableAllStatistics()

        # LSQ
        cpu_lsq = cpu.setSubComponent("lsq", "vanadis.VanadisBasicLoadStoreQueue")
        cpu_lsq.addParams(lsqParams)
        cpu_lsq.enableAllStatistics()


        # Memory interfaces
        cpuDcacheIf = cpu_lsq.setSubComponent("memory_interface", "memHierarchy.standardInterface")
        cpuIcacheIf = cpu.setSubComponent("mem_interface_inst", "memHierarchy.standardInterface")

        # L1 D-cache
        cpu_l1dcache = sst.Component(prefix + ".l1dcache", "memHierarchy.Cache")
        cpu_l1dcache.addParams(l1dcacheParams)
        l1dcache_2_cpu = cpu_l1dcache.setSubComponent("cpulink", "memHierarchy.MemLink")
        l1dcache_2_l2cache = cpu_l1dcache.setSubComponent("memlink", "memHierarchy.MemLink")

        # L1 I-cache
        cpu_l1icache = sst.Component(prefix + ".l1icache", "memHierarchy.Cache")
        cpu_l1icache.addParams(l1icacheParams)
        l1icache_2_cpu = cpu_l1icache.setSubComponent("cpulink", "memHierarchy.MemLink")
        l1icache_2_l2cache = cpu_l1icache.setSubComponent("memlink", "memHierarchy.MemLink")

        # L2 cache
        cpu_l2cache = sst.Component(prefix + ".l2cache", "memHierarchy.Cache")
        cpu_l2cache.addParams(l2cacheParams)
        l2cache_2_l1caches = cpu_l2cache.setSubComponent("cpulink", "memHierarchy.MemLink")
        l2cache_2_mem = cpu_l2cache.setSubComponent("memlink", "memHierarchy.MemNIC")
        l2cache_2_mem.addParams(l2memLinkParams)

        # L1 to L2 bus
        cache_bus = sst.Component(prefix + ".bus", "memHierarchy.Bus")
        cache_bus.addParams(busParams)

        # ** Give each CPU its own processor bus name to avoid conflicts **
        processor_bus = sst.Component(prefix + ".processorBus", "memHierarchy.Bus")
        processor_bus.addParams(busParams)

        # Data TLB
        dtlbWrapper = sst.Component(prefix + ".dtlb", "mmu.tlb_wrapper")
        dtlbWrapper.addParams(tlbWrapperParams)
        dtlb = dtlbWrapper.setSubComponent("tlb", "mmu." + tlbType)
        dtlb.addParams(tlbParams)

        # Instruction TLB
        itlbWrapper = sst.Component(prefix + ".itlb", "mmu.tlb_wrapper")
        itlbWrapper.addParams(tlbWrapperParams)
        itlbWrapper.addParam("exe", True)
        itlb = itlbWrapper.setSubComponent("tlb", "mmu." + tlbType)
        itlb.addParams(tlbParams)

        # Links
        link_lsq_l1dcache_link = sst.Link(prefix + ".link_cpu_dbus_link")
        link_lsq_l1dcache_link.connect(
            (cpuDcacheIf, "port", "1ns"),
            (processor_bus, "high_network_0", "1ns"),
        )
        link_lsq_l1dcache_link.setNoCut()

        link_bus_l1cache_link = sst.Link(prefix + ".link_bus_l1cache_link")
        link_bus_l1cache_link.connect(
            (processor_bus, "low_network_0", "1ns"), (dtlbWrapper, "cpu_if", "1ns")
        )
        link_bus_l1cache_link.setNoCut()

        link_cpu_l1dcache_link = sst.Link(prefix + ".link_cpu_l1dcache_link")
        link_cpu_l1dcache_link.connect(
            (dtlbWrapper, "cache_if", "1ns"), (l1dcache_2_cpu, "port", "1ns")
        )
        link_cpu_l1dcache_link.setNoCut()

        link_cpu_itlb_link = sst.Link(prefix + ".link_cpu_itlb_link")
        link_cpu_itlb_link.connect((cpuIcacheIf, "port", "1ns"), (itlbWrapper, "cpu_if", "1ns"))
        link_cpu_itlb_link.setNoCut()

        link_cpu_l1icache_link = sst.Link(prefix + ".link_cpu_l1icache_link")
        link_cpu_l1icache_link.connect(
            (itlbWrapper, "cache_if", "1ns"), (l1icache_2_cpu, "port", "1ns")
        )
        link_cpu_l1icache_link.setNoCut()

        link_l1dcache_l2cache_link = sst.Link(prefix + ".link_l1dcache_l2cache_link")
        link_l1dcache_l2cache_link.connect(
            (l1dcache_2_l2cache, "port", "1ns"),
            (cache_bus, "high_network_0", "1ns"),
        )
        link_l1dcache_l2cache_link.setNoCut()

        link_l1icache_l2cache_link = sst.Link(prefix + ".link_l1icache_l2cache_link")
        link_l1icache_l2cache_link.connect(
            (l1icache_2_l2cache, "port", "1ns"),
            (cache_bus, "high_network_1", "1ns"),
        )
        link_l1icache_l2cache_link.setNoCut()

        link_bus_l2cache_link = sst.Link(prefix + ".link_bus_l2cache_link")
        link_bus_l2cache_link.connect(
            (cache_bus, "low_network_0", "1ns"),
            (l2cache_2_l1caches, "port", "1ns"),
        )
        link_bus_l2cache_link.setNoCut()

        return (cpu, "os_link", "5ns"), (l2cache_2_mem, "port", "1ns"), (dtlb, "mmu", "1ns"), (itlb, "mmu", "1ns")


def addParamsPrefix(prefix, params):
    ret = {}
    for key, value in params.items():
        ret[prefix + "." + key] = value
    return ret


# Node OS
node_os = sst.Component("os", "vanadis.VanadisNodeOS")
node_os.addParams(osParams)

num = 0
for i, process in processList:
    for y in range(i):
        node_os.addParams(addParamsPrefix("process" + str(num), process))
        num += 1

if pythonDebug:
    print("total hardware threads " + str(num))

# Node OS MMU
node_os_mmu = node_os.setSubComponent("mmu", "mmu." + mmuType)
node_os_mmu.addParams(mmuParams)

# Node OS memory interface
node_os_mem_if = node_os.setSubComponent("mem_interface", "memHierarchy.standardInterface")

# Node OS L1 data cache
os_cache = sst.Component("node_os.cache", "memHierarchy.Cache")
os_cache.addParams(osl1cacheParams)
os_cache_2_cpu = os_cache.setSubComponent("cpulink", "memHierarchy.MemLink")
os_cache_2_mem = os_cache.setSubComponent("memlink", "memHierarchy.MemNIC")
os_cache_2_mem.addParams(l2memLinkParams)

# Node memory router
comp_chiprtr = sst.Component("chiprtr", "merlin.hr_router")
comp_chiprtr.addParams(memRtrParams)
comp_chiprtr.setSubComponent("topology", "merlin.singlerouter")

# Directory controller
dirctrl = sst.Component("dirctrl", "memHierarchy.DirectoryController")
dirctrl.addParams(dirCtrlParams)
dirtoM = dirctrl.setSubComponent("memlink", "memHierarchy.MemLink")
dirNIC = dirctrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
dirNIC.addParams(dirNicParams)

# Memory controller
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams(memCtrlParams)
memToDir = memctrl.setSubComponent("cpulink", "memHierarchy.MemLink")
memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams(memParams)

# Link directory to router
link_dir_2_rtr = sst.Link("link_dir_2_rtr")
link_dir_2_rtr.connect((comp_chiprtr, "port" + str(numCpus), "1ns"), (dirNIC, "port", "1ns"))
link_dir_2_rtr.setNoCut()

# Link directory to memory
link_dir_2_mem = sst.Link("link_dir_2_mem")
link_dir_2_mem.connect((dirtoM, "port", "1ns"), (memToDir, "port", "1ns"))
link_dir_2_mem.setNoCut()

# OS -> OS Cache
link_os_cache_link = sst.Link("link_os_cache_link")
link_os_cache_link.connect((node_os_mem_if, "port", "1ns"), (os_cache_2_cpu, "port", "1ns"))
link_os_cache_link.setNoCut()

# OS Cache -> router
os_cache_2_rtr = sst.Link("os_cache_2_rtr")
os_cache_2_rtr.connect((os_cache_2_mem, "port", "1ns"), (comp_chiprtr, "port" + str(numCpus + 1), "1ns"))
os_cache_2_rtr.setNoCut()

# Build CPUs
cpuBuilder = CPU_Builder()
nodeId = 0
for cpu in range(numCpus):
    prefix = "node" + str(nodeId) + ".cpu" + str(cpu)
    os_hdlr, l2cache, dtlb, itlb = cpuBuilder.build(prefix, nodeId, cpu)

    link_mmu_dtlb_link = sst.Link(prefix + ".link_mmu_dtlb_link")
    link_mmu_dtlb_link.connect((node_os_mmu, "core" + str(cpu) + ".dtlb", "1ns"), dtlb)

    link_mmu_itlb_link = sst.Link(prefix + ".link_mmu_itlb_link")
    link_mmu_itlb_link.connect((node_os_mmu, "core" + str(cpu) + ".itlb", "1ns"), itlb)

    link_core_os_link = sst.Link(prefix + ".link_core_os_link")
    link_core_os_link.connect(os_hdlr, (node_os, "core" + str(cpu), "5ns"))

    link_l2cache_2_rtr = sst.Link(prefix + ".link_l2cache_2_rtr")
    link_l2cache_2_rtr.connect(l2cache, (comp_chiprtr, "port" + str(cpu), "1ns"))
