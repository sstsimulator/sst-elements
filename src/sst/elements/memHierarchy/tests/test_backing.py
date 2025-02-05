import sst
import sys
from mhlib import Bus


# Test backing initialization (init()) and read/write to files (mmap & malloc)
# Write malloc output files is tested in test_coherence* tests
#
# Test 0: Write an mmap output file, check that file matches ref
# Test 1: Read mmap input file, do 0 operations, and check that output matches ref
# Test 2: Read mmap input file, write the same file (mmap instead of copy in to out file) and check that output matches ref
# Test 3: Write malloc output file, check that file matches ref
# Test 4: Read malloc input file, do 0 operations, and check that output matches ref
# Test 5: Write backing during init(), do 0 operations, check that output matches ref
# Test hierarchy includes Bus (MemLink) and Network (MemNIC) to ensure both link types behave as expected

DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_L3 = 0
DEBUG_DC = 0
DEBUG_MEM = 0
DEBUG_NOC = 0

option = 0;
if len(sys.argv) < 8:
    print("Argument count is incorrect. Required: <test_case> <seed0> <seed1> <seed2> <seed3> <infile> <outfile>")
    exit(0)

option = int(sys.argv[1])
seeds = [int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4]), int(sys.argv[5])]
infile = sys.argv[6]
outfile = sys.argv[7]

ops = 0
if option == 0 or option == 2 or option == 3:
    ops = 75

cpu_params = {
    "memFreq" : 1,
    "memSize" : "24KiB",
    "verbose" : 0,
    "clock" : "3GHz",
    "maxOutstanding" : 16,
    "opCount" : ops,
    "reqsPerIssue" : 4,
}

if option == 5:
    cpu_params["test_init"] = 8

# Core 0
cpu0 = sst.Component("core0", "memHierarchy.standardCPU")
cpu0.addParams(cpu_params)
cpu0.addParams({
    "rngseed" : seeds[0],
    "write_freq" : 40, # 40% writes
    "read_freq" : 59,  # 59% reads
    "flushcache_freq" : 1, # 1% FlushAll
})
cpu0_iface = cpu0.setSubComponent("memory", "memHierarchy.standardInterface")

# Core 1
cpu1 = sst.Component("core1", "memHierarchy.standardCPU")
cpu1.addParams(cpu_params)
cpu1.addParams({
    "rngseed" : seeds[1],
    "write_freq" : 34, # 34% writes
    "read_freq" : 65,  # 65% reads
    "flushcache_freq" : 1, # 1% FlushAll
})
cpu1_iface = cpu1.setSubComponent("memory", "memHierarchy.standardInterface")

# Core 2
cpu2 = sst.Component("core2", "memHierarchy.standardCPU")
cpu2.addParams(cpu_params)
cpu2.addParams({
    "rngseed" : seeds[2],
    "write_freq" : 45, # 45% writes
    "read_freq" : 55,  # 55% reads
})
cpu2_iface = cpu2.setSubComponent("memory", "memHierarchy.standardInterface")

# Core 3
cpu3 = sst.Component("core3", "memHierarchy.standardCPU")
cpu3.addParams(cpu_params)
cpu3.addParams({
    "rngseed" : seeds[3],
    "read_freq" : 100,  # 100% reads
})
cpu3_iface = cpu3.setSubComponent("memory", "memHierarchy.standardInterface")


# L1s
protocol = "mesi"

l1cache_params = {
      "access_latency_cycles" : 1,
      "cache_frequency" : "3Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : protocol,
      "associativity" : 4,
      "cache_line_size" : "64",
      "mshr_num_entries" : 16,
      "debug_level" : "10",
      "debug" : DEBUG_L1,
      "L1" : "1",
      "cache_size" : "1 KiB"
}

l1_cache0 = sst.Component("l1cache0", "memHierarchy.Cache")
l1_cache0.addParams(l1cache_params)
l1_cache1 = sst.Component("l1cache1", "memHierarchy.Cache")
l1_cache1.addParams(l1cache_params)
l1_cache2 = sst.Component("l1cache2", "memHierarchy.Cache")
l1_cache2.addParams(l1cache_params)
l1_cache3 = sst.Component("l1cache3", "memHierarchy.Cache")
l1_cache3.addParams(l1cache_params)

link0_cpu_l1 = sst.Link("link0_cpu_l1")
link0_cpu_l1.connect( (cpu0_iface, "lowlink", "100ps"), (l1_cache0, "highlink", "100ps") )
link1_cpu_l1 = sst.Link("link1_cpu_l1")
link1_cpu_l1.connect( (cpu1_iface, "lowlink", "100ps"), (l1_cache1, "highlink", "100ps") )
link2_cpu_l1 = sst.Link("link2_cpu_l1")
link2_cpu_l1.connect( (cpu2_iface, "lowlink", "100ps"), (l1_cache2, "highlink", "100ps") )
link3_cpu_l1 = sst.Link("link3_cpu_l1")
link3_cpu_l1.connect( (cpu3_iface, "lowlink", "100ps"), (l1_cache3, "highlink", "100ps") )

# Memory
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : 10,
    "clock" : "1GHz",
    "addr_range_end" : 24*1024-1,
#    "backing_out_screen" : True,
})

# Test 0: Write an mmap output file, check that file matches ref
# Test 1: Read mmap input file, do 0 operations, and check that output matches ref
# Test 2: Read mmap input file, write the same file (mmap instead of copy in to out file) and check that output matches ref
# Test 3: Write malloc output file, check that file matches ref
# Test 4: Read malloc input file, do 0 operations, and check that output matches ref
# Test 5: Write backing during init(), do 0 operations, check that output matches ref
if option < 3:
    memctrl.addParams({ "backing" : "mmap", "backing_init_zero" : True, "backing_out_file" : outfile })
else:
    memctrl.addParams({ "backing" : "malloc", "backing_init_zero" : True, "backing_out_file" : outfile })

if option == 1 or option == 2 or option == 4:
    memctrl.addParam("backing_in_file", infile)

memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
      "mem_size" : "24KiB",
      "access_time" : "80ns",
})

##################
## Hierarchy is
##  L2 inclusive/private
##  L3 inclusive/shared (req bus between L2 & L3)
##  Directory (2x)
##  Memory (1x)
## Network for L3/dir/mem

#########################
## Construct L2 level
#########################

l2cache_base_params = {
    "access_latency_cycles" : 3,
    "cache_frequency" : "3GHz",
    "replacement_policy" : "lru",
    "cache_size" : "2KiB",
    "coherence_protocol" : protocol,
    "associativity" : "8",
    "cache_line_size" : "64",
    "mshr_num_entries" : 24,
    "debug_level" : "10",
    "debug" : DEBUG_L2,
}

l2_cache0 = sst.Component("l2cache0", "memHierarchy.Cache")
l2_cache1 = sst.Component("l2cache1", "memHierarchy.Cache")
l2_cache2 = sst.Component("l2cache2", "memHierarchy.Cache")
l2_cache3 = sst.Component("l2cache3", "memHierarchy.Cache")
l2_cache0.addParams(l2cache_base_params)
l2_cache1.addParams(l2cache_base_params)
l2_cache2.addParams(l2cache_base_params)
l2_cache3.addParams(l2cache_base_params)
# Create and connect L2s to bus
bus_params = {"bus_frequency" : "3GHz"}
l2_bus = Bus("l2bus", bus_params, "100ps", [l2_cache0, l2_cache1, l2_cache2, l2_cache3])

# Connect L1s to L2s
link0_l1_l2 = sst.Link("link0_l1_l2")
link1_l1_l2 = sst.Link("link1_l1_l2")
link2_l1_l2 = sst.Link("link2_l1_l2")
link3_l1_l2 = sst.Link("link3_l1_l2")
link0_l1_l2.connect((l1_cache0, "lowlink", "100ps"), (l2_cache0, "highlink", "100ps"))
link1_l1_l2.connect((l1_cache1, "lowlink", "100ps"), (l2_cache1, "highlink", "100ps"))
link2_l1_l2.connect((l1_cache2, "lowlink", "100ps"), (l2_cache2, "highlink", "100ps"))
link3_l1_l2.connect((l1_cache3, "lowlink", "100ps"), (l2_cache3, "highlink", "100ps"))
    

l3cache_base_params = {
    "access_latency_cycles" : 7,
    "cache_frequency" : "3 Ghz",
    "replacement_policy" : "lru",
    "coherence_protocol" : protocol,
    "cache_size" : "12KiB",
    "associativity" : "8",
    "mshr_num_entries" : 48,
    "cache_line_size" : "64",
    "debug_level" : "10",
    "debug" : DEBUG_L3,
}

l3_cache = sst.Component("l3cache", "memHierarchy.Cache")
l3_cache.addParams(l3cache_base_params)
l2_bus.connect(lowcomps=[l3_cache])

dir_base_params = {
    "clock" : "2GHz",
    "entry_cache_size" : 512,
    "coherence_protocol" : protocol,
    "mshr_num_entries" : 256,
    "access_latency_cycles" : 1,
    "mshr_latency_cycles" : 1,
    "max_requests_per_cycle" : 2,
    "debug_level" : "10",
    "debug" : DEBUG_DC,
}

directory0 = sst.Component("directory0", "memHierarchy.DirectoryController")
directory1 = sst.Component("directory1", "memHierarchy.DirectoryController")
directory0.addParams(dir_base_params)
directory1.addParams(dir_base_params)
directory0.addParams({"addr_range_start" : 0, "addr_range_end" : 12*1024-1}) # Addrs 0-12K
directory1.addParams({"addr_range_start" : 12*1024, "addr_range_end" : 24*1024-1}) # Addrs 12K-24K

## Configure NoC
noc = sst.Component("chiprtr", "merlin.hr_router")
noc.addParams({
    "xbar_bw" : "1GB/s",
    "link_bw" : "1GB/s",
    "input_buf_size" : "1KB",
    "num_ports" : "4",
    "flit_size" : "72B",
    "output_buf_size" : "1KB",
    "id" : 0,
    })
noc.setSubComponent("topology", "merlin.singlerouter")

l3_nic = l3_cache.setSubComponent("lowlink", "memHierarchy.MemNIC")
dir0_nic = directory0.setSubComponent("highlink", "memHierarchy.MemNIC")
dir1_nic = directory1.setSubComponent("highlink", "memHierarchy.MemNIC")
mem_nic = memctrl.setSubComponent("highlink", "memHierarchy.MemNIC")

l3_nic.addParams({"group" : 1, "network_bw" : "2GB/s", "debug" : DEBUG_NOC, "debug_level" : 10})
dir0_nic.addParams({"group" : 2, "network_bw" : "2GB/s", "debug" : DEBUG_NOC, "debug_level" : 10})
dir1_nic.addParams({"group" : 2, "network_bw" : "2GB/s", "debug" : DEBUG_NOC, "debug_level" : 10})
mem_nic.addParams({"group" : 3, "network_bw" : "2GB/s", "debug" : DEBUG_NOC, "debug_level" : 10})

l3_net_link = sst.Link("l3_net_link")
d0_net_link = sst.Link("d0_net_link")
d1_net_link = sst.Link("d1_net_link")
mem_net_link = sst.Link("me_net_link")
l3_net_link.connect((l3_nic, "port", "100ps"), (noc, "port0", "100ps"))
d0_net_link.connect((dir0_nic, "port", "100ps"), (noc, "port1", "100ps"))
d1_net_link.connect((dir1_nic, "port", "100ps"), (noc, "port2", "100ps"))
mem_net_link.connect((mem_nic, "port", "100ps"), (noc, "port3", "100ps"))

