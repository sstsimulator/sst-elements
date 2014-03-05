import sst
import os

sst.setProgramOption("timebase", "1ns")

sst_root = os.getenv( "SST_ROOT" )
ariel = sst.Component("a0", "ariel.ariel")
ariel.addParams({
        "verbose" : "1",
        "maxcorequeue" : "256",
        "maxissuepercycle" : "2",
        "pipetimeout" : "0",
        "executable" : sst_root + "/sst/elements/ariel/tool/test/tlvlstream/ministream",
        "arielmode" : "1",
        "arieltool" : sst_root + "/sst/elements/ariel/tool/arieltool.so",
	"memorylevels" : "2",
	"defaultlevel" : "1"
        })

corecount = 1;

membus = sst.Component("membus", "memHierarchy.Bus")

membus.addParams({
                "numPorts" : str(corecount + 1),
                "busDelay" : "1ns"
        })

l1cache = sst.Component("l1cache", "memHierarchy.Cache")
l1cache.addParams({
	"cache_frequency" : "2GHz",
	"associativity" : "2",
	"cache_size" : "4KB",
        "cache_line_size" : "64",
	"mshr_num_entries" : "1",
	"high_network_links" : "1",
	"low_network_links" : "1",
	"access_latency_cycles" : "5",
        "access_time" : "1ns",
        "num_upstream" : "1",
        "printStats" : "1",
	"L1" : "1"
	})

memory = sst.Component("memory", "memHierarchy.MemController")
memory.addParams({
        "access_time" : "10ns",
        "mem_size" : "2048",
        "clock" : "1GHz",
        "use_dramsim" : "0",
        "device_ini" : "DDR3_micron_32M_8B_x4_sg125.ini",
        "system_ini" : "system.ini",
        })

cpu_cache_link = sst.Link("cpu_cache_link")
cpu_cache_link.connect( (ariel, "cache_link_0", "50ps"), (l1cache, "upstream0", "50ps") )

cache_bus_link = sst.Link("cache_bus_link")
cache_bus_link.connect( (membus, "port0", "50ps"), (l1cache, "snoop_link", "50ps") )

memory_link = sst.Link("mem_bus_link")
memory_link.connect( (membus, "port1", "50ps"), (memory, "snoop_link", "50ps") )
