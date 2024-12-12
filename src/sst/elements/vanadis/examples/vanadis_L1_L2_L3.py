import os
import sst

mh_debug_level=10
mh_debug=0

verbose = 2

DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_L3 = 0
DEBUG_MEM = 0
DEBUG_LEVEL = 10

cpu_clock = "2.3GHz"
protocol="MESI"

# create the operating system for the node we are modeling 
node_os = sst.Component("nodeOS", "vanadis.VanadisNodeOS")
node_os.addParams({
    "processDebugLevel": 0,
    "dbgLevel": 0,
    "dbgMask": 8,
    "cores": 1,
    "hardwareThreadCount": 1,
    "page_size": 4096,
    "physMemSize": "4GiB",
    "useMMU": True,
    "process1.env_count": 1,
    "process1.env0": "OMP_NUM_THREADS=1",
    "process1.exe": "../tests/small/basic-io/hello-world/riscv64/hello-world",
    "process1.arg0": "hello-world"
})

# create the MMU for the node's OS
node_os_mmu = node_os.setSubComponent("mmu", "mmu.simpleMMU")
node_os_mmu.addParams({
    "debug_level": 0,
    "num_cores": 1,
    "num_threads": 1,
    "page_size": 4096
})


# connect the node to it's memory
node_memory_interface = node_os.setSubComponent("mem_interface", "memHierarchy.standardInterface")

os_cache = sst.Component("node_os.cache", "memHierarchy.Cache")
os_cache.addParams({
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
})

os_cache_cpu_link = os_cache.setSubComponent("cpulink", "memHierarchy.MemLink")
os_cache_mem_link = os_cache.setSubComponent("memlink", "memHIerarchy.MemNIC")
os_cache_mem_link.addParams({ 
    "group" : 1,
    "network_bw" : "25GB/s" 
})

cpu = sst.Component("nodeCPU", "vanadis.dbg_VanadisCPU") # if this is a parameter, it determines whether this should be in memh or vanadis
cpu.addParams({
    "memFreq": 2,    
    "memSize": "4KiB",
    "verbose": 0,
    "clock": "3.5GHz",
    "rngseed": 111,
    "maxOutstanding": 16,
    "opCount": 2500,
    "reqsPerIssue": 3,
    "write_freq": 36,
    "read_freq": 60,
    "llsc_Freq": 4,
    "core_id": 1
})

decoder = sst.Component("nodeCPUDecoder", "vanadis.VanadisRISCV64Decoder")
decoder.addParams({
    "loader_mode": 0,
    "uop_cache_entries" : 1536,
    "predecode_cache_entries" : 4
})

os_handler = decoder.setSubComponent("os_handler", "vanadis.VanadisRISCV64Decoder")

branch_unit = decoder.setSubComponent("branch_unit", "vanadis.VanadisBasicBranchUnit")
branch_unit.addParams({
    "branch_entries" : 32
})

cpu_lsq = cpu.setSubComponent("lsq", "VanadisBasicLoadStoreQueue")
cpu_lsq.addParams({
    "verbose" : verbose,
    "address_mask" : 0xFFFFFFFF,
    "max_stores" : 16,
    "max_loads" : 8
})

d_cache_interface = cpu_lsq.setSubComponent("memory_interface", "memHierarchy.standardInterface")

i_cache_interface = cpu.setSubComponent("mem_interface_inst", "memHierarchy.standardInterface")

l1_d_cache = sst.Component("nodeCPU.l1dcache", "memHierarchy.Cache")
l1_d_cache.addParams({
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
})

cpu_l1_d_cache_link = l1_d_cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l1_d_cache_l2_cache_link = l1_d_cache.setSubComponent("memlink", "memHierarchy.MemLink")

# dtlbWrapper = sst.Component("dtlb", "mmu.tlb_wrapper")
# dtlbWrapper.addParams({
#     "debug_lebel": 0
# })

# dtlb = dtlbWrapper.setSubcomponent("tlb", "mmu.simpleTLB")
# dtlb.addParams({
#     "debug_level": 0,
#     "hitLatency": 1,
#     "num_hardware_threads": 1,
#     "num_tlb_entries_per_thread": 64,
#     "tlb_set_size": 4
# })

# itlbWrapper = sst.Component("itlb", "mmu.tlb_wrapper")
# itlbWrapper.addParams({
#     "debug_level": 0,
#     "exe": True
# })

# itlb = itlbWrapper.setSubComponent("tlb", "mmu.simpleTLB")
# itlb.addParams({
#     "debug_level": 0,
#     "hitLatency": 1,
#     "num_hardware_threads": 1,
#     "num_tlb_entries_per_thread": 64,
#     "tlb_set_size": 4
# })

# # connect the CPU to the TLB
# cpu_dtlb_link = sst.Link("cpu_dtlb_link")
# cpu_dtlb_link.connect((d_cache_interface, "port", "1ns"), (dtlbWrapper, "cpu_if", "1ns"))
# cpu_dtlb_link.setNoCut()

# # connect data TLB to L1 D cache
# cput_l1dcache_link = sst.Link("cpu_l1dcache_link")
# cpu_l1dcache_link.connect((dtlbWrapper, "cache_if", "1ns"), (cpu_l1_d_cache_link, "port", "1ns"))

l1_i_cache = sst.Component("nodeCPU.l1icache", "memHierarchy.Cache")
l1_i_cache.addParams({
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
})

cpu_l1_i_cache_link = l1_i_cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l1_i_cache_l2_cache_link = l1_i_cache.setSubComponent("memlink", "memHierarchy.MemLink")

l2_cache = sst.Component("nodeCPU.l2cache", "memHierarchy.Cache")
l2_cache.addParams({
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

l2_cache_l1_cache_link = l2_cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l2_cache_l3_cache_link = l2_cache.setSubComponent("memlink", "memHierarchy.MemLink")

cache_bus = sst.Component("nodeCPU.bus", "memHierarchy.Bus")
cache_bus.addParams({
    "bus_frequency": cpu_clock
})

data_tlb_wrapper = sst.Component("nodeCPU.dtlb", "mmu.tlb_wrapper")
data_tlb_wrapper.addParams({
    "debug_level": 0,
})

data_tlb = data_tlb_wrapper.setSubComponent("tlb", "mmu.simpleTLB")
data_tlb.addParams({
    "debug_level": 0,
    "hitLatency": 1,
    "num_hardware_threads": 1,
    "num_tlb_entries_per_thread": 64,
    "tlb_set_size": 4,
})

cpu_data_tlb_link = sst.Link("nodeCPU.link_cpu_dtlb_link")
cpu_data_tlb_link.connect((d_cache_interface, "port", "1ns"), (data_tlb_wrapper, "cpu_if", "1ns"))
cpu_data_tlb_link.setNoCut()

data_tlb_l1_d_cache_link = sst.Link("nodeCPU.link_cpu_l1dcache_link")
data_tlb_l1_d_cache_link.connect((data_tlb_wrapper, "cache_if", "1ns"), (cpu_l1_d_cache_link, "port", "1ns"))
data_tlb_l1_d_cache_link.setNoCut()

instruction_tlb_wrapper = sst.Component("nodeCPU.itlb", "mmu.tlb_wrapper")
instruction_tlb_wrapper.addParams({
    "debug_level": 0,
    "exe": True
})

instruction_tlb = instruction_tlb_wrapper.setSubComponent("tlb", "mmu.simpleTLB")
instruction_tlb.addParams({
    "debug_level": 0,
    "hitLatency": 1,
    "num_hardware_threads": 1,
    "num_tlb_entries_per_thread": 64,
    "tlb_set_size": 4,
})

cpu_instruction_tlb_link = sst.Link("nodeCPU.link_cpu_itlb_link")
cpu_instruction_tlb_link.connect((i_cache_interface, "port", "1ns"), (instruction_tlb_wrapper, "cpu_if", "1ns"))
cpu_instruction_tlb_link.setNoCut()

instruction_tlb_l1_i_cache_link = sst.Link("nodeCPU.link_cpu_l1icache_link")
instruction_tlb_l1_i_cache_link.connect((instruction_tlb_wrapper, "cache_if", "1ns"), (cpu_l1_i_cache_link, "port", "1ns"))
instruction_tlb_l1_i_cache_link.setNoCut()

l1_d_cache_cache_bus_link = sst.Link("nodeCPU.l1_d_cache_bus_link")
l1_d_cache_cache_bus_link.connect((l1_d_cache_l2_cache_link, "port", "1ns"), (cache_bus, "high_network_0", "1ns"))
l1_d_cache_cache_bus_link.setNoCut()

l1_i_cache_cache_bus_link = sst.Link("nodeCPU.l1_i_cache_bus_link")
l1_i_cache_cache_bus_link.connect((l1_i_cache_l2_cache_link, "port", "1ns"), (cache_bus, "high_network_1", "1ns"))
l1_i_cache_cache_bus_link.setNoCut()

cache_bus_l2_cache_link = sst.Link("nodeCPU.bus_l2_cache_link")
cache_bus_l2_cache_link.connect((cache_bus, "low_network_0", "1ns"), (l2_cache_l1_cache_link, "port", "1ns"))
cache_bus_l2_cache_link.setNoCut()

l3_cache = sst.Component("nodeCPU.l3cache", "memHierarchy.Cache")
l3_cache.addParams({
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

l3_cache_l2_cache = l3_cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l3_cache_mem_controller = l3_cache.setSubComponent("memlink", "memHierarchy.MemLink")

mem_controller = sst.Component("memory", "memHierarchy.MemController")
mem_controller.addParams({
    "clock" : cpu_clock,
    "backend.mem_size" : "4GiB",
    "backing" : "malloc",
    "initBacking": 1,
    "addr_range_start": 0,
    "addr_range_end": 0xffffffff,
    "debug_level" : mh_debug_level,
    "debug" : mh_debug,
})

l3_cache_l2_cache_link = sst.Link("nodeCPU.l2_cache_l3_cache_link")
l3_cache_l2_cache_link.connect((l2_cache_l3_cache_link, "port", "1ns"), (l3_cache_l2_cache, "port", "1ns"))
l3_cache_l2_cache_link.setNoCut()

l3_cache_mem_controller_link = sst.Link("nodeCPU.l3_cache_mem_controller_link")
l3_cache_mem_controller_link.connect((l3_cache_mem_controller, "port", "1ns"), (mem_controller, "direct_link", "1ns"))
l3_cache_mem_controller_link.setNoCut()

mmu_dtlb_link = sst.Link("mmu_dtlb_link")
mmu_dtlb_link.connect((node_os_mmu, "core0.dtlb", "1ns"), (data_tlb, "mmu", "1ns"))

mmu_itlb_link = sst.Link("mmu_itlb_link")
mmu_itlb_link.connect((node_os_mmu, "core0.itlb", "1ns"), (instruction_tlb, "mmu", "1ns"))

core_os_link = sst.Link("core_os_link")
core_os_link.connect((cpu, "os_link", "5ns"), (node_os, "core0", "5ns"))

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
