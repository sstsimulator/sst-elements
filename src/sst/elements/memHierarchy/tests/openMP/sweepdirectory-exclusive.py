# Automatically generated SST Python input
import sst
from sst.merlin import *

import sys,getopt
import os

L1cachesz = "8 KB"
L2cachesz = "32 KB"
L3cachesz = "64 KB"
L1assoc = 2
L2assoc = 16
L3assoc = 16
Replacp = "lru"
L2MSHR = 16
MSIMESI = "MESI"
Pref1 = ""
Pref2 = ""
Executable = os.getenv('OMP_EXE', "ompbarrier/ompbarrier.x")
L1debug = 0
L2debug = 0
L3debug = 0
Dirdebug = 0
Memdebug = 0
L1debug_lev = 10
L2debug_lev = 10
L3debug_lev = 10
Dirdebug_lev = 10
Memdebug_lev = 10
addr = 0

def main():
    global L1cachesz
    global L2cachesz
    global L1assoc
    global L2assoc
    global Replacp
    global L2MSHR
    global MSIMESI
    global Pref1
    global Pref2
    global Executable

    try:
        opts, args = getopt.getopt(sys.argv[1:], "", ["L1cachesz=","L2cachesz=","L1assoc=","L2assoc=","Replacp=","L2MSHR=","MSIMESI=","Pref1=","Pref2=","Executable="])
    except getopt.GetopError as err:
        print (str(err))
        sys.exit(2)
    for o, a in opts:
        if o in ("--L1cachesz"):
            L1cachesz = a
        elif o in ("--L2cachesz"):
            L2cachesz = a
        elif o in ("--L1assoc"):
            L1assoc = a
        elif o in ("--L2assoc"):
            L2assoc = a
        elif o in ("--Replacp"):
            Replacp = a
        elif o in ("--L2MSHR"):
            L2MSHR = a
        elif o in ("--MSIMESI"):
            MSIMESI = a
        elif o in ("--Pref1"):
            if a == "yes":
                Pref1 = "cassini.NextBlockPrefetcher"
        elif o in ("--Pref2"):
            if a == "yes":
                Pref2 = "cassini.NextBlockPrefetcher"
        elif o in ("--Executable"):
                Executable = a
        else:
            print (o)
            assert False, "Unknown Options"
    print (L1cachesz, L2cachesz, L1assoc, L2assoc, Replacp, L2MSHR, MSIMESI, Pref1, Pref2, Executable)

#main()
# Define needed environment params
os.environ['OMP_NUM_THREADS']="8"

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "100ms")

# Define the simulation components
ariel_cpus = sst.Component("cpus", "ariel.ariel")
ariel_cpus.addParams({
    "verbose"           : 0,
    "clock"             : "2 Ghz",
    "maxcorequeue"      : 256,
    "maxissuepercycle"  : 4,
    "pipetimeout"       : 0,
    "corecount"         : 8,
    "arielmode"         : 1,
    "launchparamcount"  : 1,
    "launchparam0"      : "-ifeellucky",
    "memmgr.memorylevels"      : 1,
    "memmgr.pagecount0"        : 262144,
    "memmgr.defaultlevel"      : 0,
    "executable"        : Executable
})
comp_c0_l1Dcache = sst.Component("c0.l1Dcache", "memHierarchy.Cache")
comp_c0_l1Dcache.addParams({
    "debug" : L1debug,
    "access_latency_cycles" : """2""",
    "cache_frequency" : """2 Ghz""",
    "replacement_policy" : Replacp,
    "coherence_protocol" : MSIMESI,
    "associativity" : L1assoc,
    "cache_line_size" : """64""",
    "debug_level" : L1debug_lev,
    "L1" : """1""",
    "cache_size" : L1cachesz,
    "prefetcher" : Pref1
})
comp_c0_l2cache = sst.Component("c0.l2cache", "memHierarchy.Cache")
comp_c0_l2cache.addParams({
    "debug" : L2debug,
    "access_latency_cycles" : 6,
    "cache_frequency" : "2.0 Ghz",
    "replacement_policy" : Replacp,
    "coherence_protocol" : MSIMESI,
    "associativity" : L2assoc,
    "cache_type" : "noninclusive",
    "cache_line_size" : "64",
    "debug_level" : L2debug_lev,
    "cache_size" : L2cachesz,
    "mshr_num_entries" : L2MSHR,
#    "debug_addr" : addr
})
comp_c1_l1Dcache = sst.Component("c1.l1Dcache", "memHierarchy.Cache")
comp_c1_l1Dcache.addParams({
      "debug" : L1debug,
      "access_latency_cycles" : """2""",
      "cache_frequency" : """2 Ghz""",
      "replacement_policy" : Replacp,
      "coherence_protocol" : MSIMESI,
      "associativity" : L1assoc,
      "cache_line_size" : """64""",
      "debug_level" : L1debug_lev,
      "L1" : """1""",
      "cache_size" : L1cachesz,
      "prefetcher" : Pref1
})
comp_c1_l2cache = sst.Component("c1.l2cache", "memHierarchy.Cache")
comp_c1_l2cache.addParams({
    "debug" : L2debug,
    "access_latency_cycles" : 6,
    "cache_frequency" : "2.0 Ghz",
    "replacement_policy" : Replacp,
    "coherence_protocol" : MSIMESI,
    "associativity" : L2assoc,
    "cache_type" : "noninclusive",
    "cache_line_size" : "64",
    "debug_level" : L2debug_lev,
    "cache_size" : L2cachesz,
    "mshr_num_entries" : L2MSHR,
#    "debug_addr" : addr
})
comp_c2_l1Dcache = sst.Component("c2.l1Dcache", "memHierarchy.Cache")
comp_c2_l1Dcache.addParams({
      "debug" : L1debug,
      "access_latency_cycles" : """2""",
      "cache_frequency" : """2 Ghz""",
      "replacement_policy" : Replacp,
      "coherence_protocol" : MSIMESI,
      "associativity" : L1assoc,
      "cache_line_size" : """64""",
      "debug_level" : L1debug_lev,
      "L1" : """1""",
      "cache_size" : L1cachesz,
      "prefetcher" : Pref1
})
comp_c2_l2cache = sst.Component("c2.l2cache", "memHierarchy.Cache")
comp_c2_l2cache.addParams({
    "debug" : L2debug,
    "access_latency_cycles" : 6,
    "cache_frequency" : "2.0 Ghz",
    "replacement_policy" : Replacp,
    "coherence_protocol" : MSIMESI,
    "associativity" : L2assoc,
    "cache_type" : "noninclusive",
    "cache_line_size" : "64",
    "debug_level" : L2debug_lev,
    "cache_size" : L2cachesz,
    "mshr_num_entries" : L2MSHR,
#    "debug_addr" : addr
})
comp_c3_l1Dcache = sst.Component("c3.l1Dcache", "memHierarchy.Cache")
comp_c3_l1Dcache.addParams({
      "debug" : L1debug,
      "access_latency_cycles" : """2""",
      "cache_frequency" : """2 Ghz""",
      "replacement_policy" : Replacp,
      "coherence_protocol" : MSIMESI,
      "associativity" : L1assoc,
      "cache_line_size" : """64""",
      "debug_level" : L1debug_lev,
      "L1" : """1""",
      "cache_size" : L1cachesz,
      "prefetcher" : Pref1
})
comp_c3_l2cache = sst.Component("c3.l2cache", "memHierarchy.Cache")
comp_c3_l2cache.addParams({
    "debug" : L2debug,
    "access_latency_cycles" : 6,
    "cache_frequency" : "2.0 Ghz",
    "replacement_policy" : Replacp,
    "coherence_protocol" : MSIMESI,
    "associativity" : L2assoc,
    "cache_type" : "noninclusive",
    "cache_line_size" : "64",
    "debug_level" : L2debug_lev,
    "cache_size" : L2cachesz,
    "mshr_num_entries" : L2MSHR,
#    "debug_addr" : addr
})
comp_c4_l1Dcache = sst.Component("c4.l1Dcache", "memHierarchy.Cache")
comp_c4_l1Dcache.addParams({
      "debug" : L1debug,
      "access_latency_cycles" : """2""",
      "cache_frequency" : """2 Ghz""",
      "replacement_policy" : Replacp,
      "coherence_protocol" : MSIMESI,
      "associativity" : L1assoc,
      "cache_line_size" : """64""",
      "debug_level" : L1debug_lev,
      "L1" : """1""",
      "cache_size" : L1cachesz,
      "prefetcher" : Pref1
})
comp_c4_l2cache = sst.Component("c4.l2cache", "memHierarchy.Cache")
comp_c4_l2cache.addParams({
    "debug" : L2debug,
    "access_latency_cycles" : 6,
    "cache_frequency" : "2.0 Ghz",
    "replacement_policy" : Replacp,
    "coherence_protocol" : MSIMESI,
    "associativity" : L2assoc,
    "cache_type" : "noninclusive",
    "cache_line_size" : "64",
    "debug_level" : L2debug_lev,
    "cache_size" : L2cachesz,
    "mshr_num_entries" : L2MSHR,
#    "debug_addr" : addr
})
comp_c5_l1Dcache = sst.Component("c5.l1Dcache", "memHierarchy.Cache")
comp_c5_l1Dcache.addParams({
      "debug" : L1debug,
      "access_latency_cycles" : """2""",
      "cache_frequency" : """2 Ghz""",
      "replacement_policy" : Replacp,
      "coherence_protocol" : MSIMESI,
      "associativity" : L1assoc,
      "cache_line_size" : """64""",
      "debug_level" : L1debug_lev,
      "L1" : """1""",
      "cache_size" : L1cachesz,
      "prefetcher" : Pref1
})
comp_c5_l2cache = sst.Component("c5.l2cache", "memHierarchy.Cache")
comp_c5_l2cache.addParams({
    "debug" : L2debug,
    "access_latency_cycles" : 6,
    "cache_frequency" : "2.0 Ghz",
    "replacement_policy" : Replacp,
    "coherence_protocol" : MSIMESI,
    "associativity" : L2assoc,
    "cache_type" : "noninclusive",
    "cache_line_size" : "64",
    "debug_level" : L2debug_lev,
    "cache_size" : L2cachesz,
    "mshr_num_entries" : L2MSHR,
#    "debug_addr" : addr
})
comp_c6_l1Dcache = sst.Component("c6.l1Dcache", "memHierarchy.Cache")
comp_c6_l1Dcache.addParams({
      "debug" : L1debug,
      "access_latency_cycles" : """2""",
      "cache_frequency" : """2 Ghz""",
      "replacement_policy" : Replacp,
      "coherence_protocol" : MSIMESI,
      "associativity" : L1assoc,
      "cache_line_size" : """64""",
      "debug_level" : L1debug_lev,
      "L1" : """1""",
      "cache_size" : L1cachesz,
      "prefetcher" : Pref1
})
comp_c6_l2cache = sst.Component("c6.l2cache", "memHierarchy.Cache")
comp_c6_l2cache.addParams({
    "debug" : L2debug,
    "access_latency_cycles" : 6,
    "cache_frequency" : "2.0 Ghz",
    "replacement_policy" : Replacp,
    "coherence_protocol" : MSIMESI,
    "associativity" : L2assoc,
    "cache_type" : "noninclusive",
    "cache_line_size" : "64",
    "debug_level" : L2debug_lev,
    "cache_size" : L2cachesz,
    "mshr_num_entries" : L2MSHR,
#    "debug_addr" : addr
})
comp_c7_l1Dcache = sst.Component("c7.l1Dcache", "memHierarchy.Cache")
comp_c7_l1Dcache.addParams({
      "debug" : L1debug,
      "access_latency_cycles" : """2""",
      "cache_frequency" : """2 Ghz""",
      "replacement_policy" : Replacp,
      "coherence_protocol" : MSIMESI,
      "associativity" : L1assoc,
      "cache_line_size" : """64""",
      "debug_level" : L1debug_lev,
      "L1" : """1""",
      "cache_size" : L1cachesz,
      "prefetcher" : Pref1
})
comp_c7_l2cache = sst.Component("c7.l2cache", "memHierarchy.Cache")
comp_c7_l2cache.addParams({
    "debug" : L2debug,
    "access_latency_cycles" : 6,
    "cache_frequency" : "2.0 Ghz",
    "replacement_policy" : Replacp,
    "coherence_protocol" : MSIMESI,
    "associativity" : L2assoc,
    "cache_type" : "noninclusive",
    "cache_line_size" : "64",
    "debug_level" : L2debug_lev,
    "cache_size" : L2cachesz,
    "mshr_num_entries" : L2MSHR,
#    "debug_addr" : addr
})

comp_n0_bus = sst.Component("n0.bus", "memHierarchy.Bus")
comp_n0_bus.addParams({
      "bus_frequency" : """2 Ghz"""
})

comp_n0_l3cache = sst.Component("n0.l3cache", "memHierarchy.Cache")
comp_n0_l3cache.addParams({
    "debug" : L3debug,
    "access_latency_cycles" : "14",
    "cache_frequency" : """2.0 Ghz""",
    "replacement_policy" : Replacp,
    "coherence_protocol" : MSIMESI,
    "associativity" : L2assoc,
    "cache_line_size" : """64""",
    "debug_level" : L3debug_lev,
    "cache_size" : L3cachesz,
    "mshr_num_entries" : L2MSHR*2,
    "prefetcher" : Pref2,
#    "debug_addr" : addr,
    "cache_type" : "noninclusive_with_directory",
    "noninclusive_directory_entries" : 4096,
    "noninclusive_directory_associativity" : 32,
})
cpulink_n0_l3cache = comp_n0_l3cache.setSubComponent("cpulink", "memHierarchy.MemLink")
memlink_n0_l3cache = comp_n0_l3cache.setSubComponent("memlink", "memHierarchy.MemNIC")
memlink_n0_l3cache.addParams({
    "group" : 0,
    "network_bw" : "25GB/s",
    "network_input_buffer_size" : "2KB",
    "network_output_buffer_size" : "2KB",
})
comp_n1_bus = sst.Component("n1.bus", "memHierarchy.Bus")
comp_n1_bus.addParams({
      "bus_frequency" : """2 Ghz"""
})

comp_n1_l3cache = sst.Component("n1.l3cache", "memHierarchy.Cache")
comp_n1_l3cache.addParams({
    "debug" : L3debug,
    "access_latency_cycles" : "14",
    "cache_frequency" : """2.0 Ghz""",
    "replacement_policy" : Replacp,
    "coherence_protocol" : MSIMESI,
    "associativity" : L2assoc,
    "cache_line_size" : """64""",
    "debug_level" : L3debug_lev,
    "cache_size" : L3cachesz,
    "mshr_num_entries" : L2MSHR*2,
    "prefetcher" : Pref2,
#    "debug_addr" : addr,
    "cache_type" : "noninclusive_with_directory",
    "noninclusive_directory_entries" : 4096,
    "noninclusive_directory_associativity" : 32,
})
cpulink_n1_l3cache = comp_n1_l3cache.setSubComponent("cpulink", "memHierarchy.MemLink")
memlink_n1_l3cache = comp_n1_l3cache.setSubComponent("memlink", "memHierarchy.MemNIC")
memlink_n1_l3cache.addParams({
    "group" : 0,
    "network_bw" : "25GB/s",
    "network_input_buffer_size" : "2KB",
    "network_output_buffer_size" : "2KB",
})
comp_chipRtr = sst.Component("chipRtr", "merlin.hr_router")
comp_chipRtr.addParams({
      "input_buf_size" : """2KB""",
      "num_ports" : """4""",
      "id" : """0""",
      "output_buf_size" : """2KB""",
      "flit_size" : """64B""",
      "xbar_bw" : """51.2 GB/s""",
      "link_bw" : """25.6 GB/s""",
})
comp_chipRtr.setSubComponent("topology", "merlin.singlerouter")
comp_dirctrl0 = sst.Component("dirctrl0", "memHierarchy.DirectoryController")
comp_dirctrl0.addParams({
      "debug" : Dirdebug,
      "debug_level" : Dirdebug_lev,
      "coherence_protocol" : MSIMESI,
      "entry_cache_size" : """16384""",
      "addr_range_start" : """0x0""",
      "addr_range_end" : """0x000FFFFF""",
      "mshr_num_entries" : "2",
})
cpulink_dirctrl0 = comp_dirctrl0.setSubComponent("cpulink", "memHierarchy.MemNIC")
memlink_dirctrl0 = comp_dirctrl0.setSubComponent("memlink", "memHierarchy.MemLink")
cpulink_dirctrl0.addParams({
    "group" : 1,
    "network_bw" : "25GB/s",
    "network_input_buffer_size" : "2KB",
    "network_output_buffer_size" : "2KB",
})
comp_memctrl0 = sst.Component("memory0", "memHierarchy.MemController")
comp_memctrl0.addParams({
      "debug" : Memdebug,
      "debug_level" : Memdebug_lev,
      "clock" : """1.6GHz""",
})
comp_memory0 = comp_memctrl0.setSubComponent("backend", "memHierarchy.simpleMem")
comp_memory0.addParams({
    "mem_size" : "512MiB",
    "access_time" : "25 ns",
})
comp_dirctrl1 = sst.Component("dirctrl1", "memHierarchy.DirectoryController")
comp_dirctrl1.addParams({
      "debug" : Dirdebug,
      "debug_level" : Dirdebug_lev,
      "coherence_protocol" : MSIMESI,
      "entry_cache_size" : """16384""",
      "addr_range_start" : """0x00100000""",
      "addr_range_end" : """0x3FFFFFFF""",
      "mshr_num_entries" : "2",
})
cpulink_dirctrl1 = comp_dirctrl1.setSubComponent("cpulink", "memHierarchy.MemNIC")
memlink_dirctrl1 = comp_dirctrl1.setSubComponent("memlink", "memHierarchy.MemLink")
cpulink_dirctrl1.addParams({
    "group" : 1,
    "network_bw" : "25GB/s",
    "network_input_buffer_size" : "2KB",
    "network_output_buffer_size" : "2KB",
})
comp_memctrl1 = sst.Component("memory1", "memHierarchy.MemController")
comp_memctrl1.addParams({
      "debug" : Memdebug,
      "debug_level" : Memdebug_lev,
      "clock" : """1.6GHz""",
})
comp_memory1 = comp_memctrl1.setSubComponent("backend", "memHierarchy.simpleMem")
comp_memory1.addParams({
    "mem_size" : "512MiB",
    "access_time" : "25 ns",
})


# Define the simulation links
link_core0_dcache = sst.Link("link_core0_dcache")
link_core0_dcache.connect( (ariel_cpus, "cache_link_0", "500ps"), (comp_c0_l1Dcache, "high_network_0", "500ps") )
link_core1_dcache = sst.Link("link_core1_dcache")
link_core1_dcache.connect( (ariel_cpus, "cache_link_1", "500ps"), (comp_c1_l1Dcache, "high_network_0", "500ps") )
link_core2_dcache = sst.Link("link_core2_dcache")
link_core2_dcache.connect( (ariel_cpus, "cache_link_2", "500ps"), (comp_c2_l1Dcache, "high_network_0", "500ps") )
link_core3_dcache = sst.Link("link_core3_dcache")
link_core3_dcache.connect( (ariel_cpus, "cache_link_3", "500ps"), (comp_c3_l1Dcache, "high_network_0", "500ps") )
link_core4_dcache = sst.Link("link_core4_dcache")
link_core4_dcache.connect( (ariel_cpus, "cache_link_4", "500ps"), (comp_c4_l1Dcache, "high_network_0", "500ps") )
link_core5_dcache = sst.Link("link_core5_dcache")
link_core5_dcache.connect( (ariel_cpus, "cache_link_5", "500ps"), (comp_c5_l1Dcache, "high_network_0", "500ps") )
link_core6_dcache = sst.Link("link_core6_dcache")
link_core6_dcache.connect( (ariel_cpus, "cache_link_6", "500ps"), (comp_c6_l1Dcache, "high_network_0", "500ps") )
link_core7_dcache = sst.Link("link_core7_dcache")
link_core7_dcache.connect( (ariel_cpus, "cache_link_7", "500ps"), (comp_c7_l1Dcache, "high_network_0", "500ps") )
link_c0dcache_c0l2cache_link = sst.Link("link_c0dcache_c0l2cache_link")
link_c0dcache_c0l2cache_link.connect( (comp_c0_l1Dcache, "low_network_0", "100ps"), (comp_c0_l2cache, "high_network_0", "100ps") )
link_c1dcache_c1l2cache_link = sst.Link("link_c1dcache_c1l2cache_link")
link_c1dcache_c1l2cache_link.connect( (comp_c1_l1Dcache, "low_network_0", "100ps"), (comp_c1_l2cache, "high_network_0", "100ps") )
link_c2dcache_c2l2cache_link = sst.Link("link_c2dcache_c2l2cache_link")
link_c2dcache_c2l2cache_link.connect( (comp_c2_l1Dcache, "low_network_0", "100ps"), (comp_c2_l2cache, "high_network_0", "100ps") )
link_c3dcache_c3l2cache_link = sst.Link("link_c3dcache_c3l2cache_link")
link_c3dcache_c3l2cache_link.connect( (comp_c3_l1Dcache, "low_network_0", "100ps"), (comp_c3_l2cache, "high_network_0", "100ps") )
link_c4dcache_c4l2cache_link = sst.Link("link_c4dcache_c4l2cache_link")
link_c4dcache_c4l2cache_link.connect( (comp_c4_l1Dcache, "low_network_0", "100ps"), (comp_c4_l2cache, "high_network_0", "100ps") )
link_c5dcache_c5l2cache_link = sst.Link("link_c5dcache_c5l2cache_link")
link_c5dcache_c5l2cache_link.connect( (comp_c5_l1Dcache, "low_network_0", "100ps"), (comp_c5_l2cache, "high_network_0", "100ps") )
link_c6dcache_c6l2cache_link = sst.Link("link_c6dcache_c6l2cache_link")
link_c6dcache_c6l2cache_link.connect( (comp_c6_l1Dcache, "low_network_0", "100ps"), (comp_c6_l2cache, "high_network_0", "100ps") )
link_c7dcache_c7l2cache_link = sst.Link("link_c7dcache_c7l2cache_link")
link_c7dcache_c7l2cache_link.connect( (comp_c7_l1Dcache, "low_network_0", "100ps"), (comp_c7_l2cache, "high_network_0", "100ps") )
link_c0l2cache_bus_link = sst.Link("link_c0l2cache_bus_link")
link_c0l2cache_bus_link.connect( (comp_c0_l2cache, "low_network_0", "100ps"), (comp_n0_bus, "high_network_0", "100ps") )
link_c1l2cache_bus_link = sst.Link("link_c1l2cache_bus_link")
link_c1l2cache_bus_link.connect( (comp_c1_l2cache, "low_network_0", "100ps"), (comp_n0_bus, "high_network_1", "100ps") )
link_c2l2cache_bus_link = sst.Link("link_c2l2cache_bus_link")
link_c2l2cache_bus_link.connect( (comp_c2_l2cache, "low_network_0", "100ps"), (comp_n0_bus, "high_network_2", "100ps") )
link_c3l2cache_bus_link = sst.Link("link_c3l2cache_bus_link")
link_c3l2cache_bus_link.connect( (comp_c3_l2cache, "low_network_0", "100ps"), (comp_n0_bus, "high_network_3", "100ps") )
link_c4l2cache_bus_link = sst.Link("link_c4l2cache_bus_link")
link_c4l2cache_bus_link.connect( (comp_c4_l2cache, "low_network_0", "100ps"), (comp_n1_bus, "high_network_0", "100ps") )
link_c5l2cache_bus_link = sst.Link("link_c5l2cache_bus_link")
link_c5l2cache_bus_link.connect( (comp_c5_l2cache, "low_network_0", "100ps"), (comp_n1_bus, "high_network_1", "100ps") )
link_c6l2cache_bus_link = sst.Link("link_c6l2cache_bus_link")
link_c6l2cache_bus_link.connect( (comp_c6_l2cache, "low_network_0", "100ps"), (comp_n1_bus, "high_network_2", "100ps") )
link_c7l2cache_bus_link = sst.Link("link_c7l2cache_bus_link")
link_c7l2cache_bus_link.connect( (comp_c7_l2cache, "low_network_0", "100ps"), (comp_n1_bus, "high_network_3", "100ps") )
link_n0bus_n0l3cache = sst.Link("link_n0bus_n0l3cache")
link_n0bus_n0l3cache.connect( (comp_n0_bus, "low_network_0", "100ps"), (cpulink_n0_l3cache, "port", "100ps") )
link_n0bus_memory = sst.Link("link_n0bus_memory")
link_n0bus_memory.connect( (memlink_n0_l3cache, "port", "100ps"), (comp_chipRtr, "port2", "1000ps") )
link_n1bus_n1l3cache = sst.Link("link_n1bus_n1l3cache")
link_n1bus_n1l3cache.connect( (comp_n1_bus, "low_network_0", "100ps"), (cpulink_n1_l3cache, "port", "100ps") )
link_n1bus_memory = sst.Link("link_n1bus_memory")
link_n1bus_memory.connect( (memlink_n1_l3cache, "port", "100ps"), (comp_chipRtr, "port3", "1000ps") )
link_dirctrl0_bus = sst.Link("link_dirctrl0_bus")
link_dirctrl0_bus.connect( (comp_chipRtr, "port0", "1000ps"), (cpulink_dirctrl0, "port", "100ps") )
link_dirctrl1_bus = sst.Link("link_dirctrl1_bus")
link_dirctrl1_bus.connect( (comp_chipRtr, "port1", "1000ps"), (cpulink_dirctrl1, "port", "100ps") )
link_dirctrl0_mem = sst.Link("link_dirctrl0_mem")
link_dirctrl0_mem.connect( (memlink_dirctrl0, "port", "100ps"), (comp_memctrl0, "direct_link", "100ps") )
link_dirctrl1_mem = sst.Link("link_dirctrl1_mem")
link_dirctrl1_mem.connect( (memlink_dirctrl1, "port", "100ps"), (comp_memctrl1, "direct_link", "100ps") )
# End of generated output.

sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
sst.enableAllStatisticsForAllComponents()

