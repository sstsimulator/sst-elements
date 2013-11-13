import sst

sst.setProgramOption("timebase", "1ns")

ariel = sst.Component("a0", "ariel.ariel")
ariel.addParams({
	"verbose" : "1",
	"maxcorequeue" : "256",
	"maxissuepercycle" : "2",
	"pipetimeout" : "0",
	"executable" : "/home/sdhammo/subversion/sst-simulator/sst/elements/ariel/pysdl/test/delayed",
	"arielmode" : "0",
	"arieltool" : "/home/sdhammo/subversion/sst-simulator/sst/elements/ariel/tool/arieltool.so"	
	})

corecount = 8;

membus = sst.Component("membus", "memHierarchy.Bus")

membus.addParams({
		"numPorts" : str(corecount + 1),
		"busDelay" : "1ns"
	})

l1dict = dict()

for x in range(0, corecount):
    new_l1 = sst.Component("l1cache_" + str(x), "memHierarchy.Cache")
    new_l1.addParams({
	"num_ways" : "8",
	"num_rows" : "128",
	"blocksize" : "64",
	"access_time" : "1ns",
	"num_upstream" : "1",
	"printStats" : "1"
	})
    l1dict["l1cache_" + str(x)] = new_l1

l1links = dict()
for x in range(0, corecount):
    new_l1_to_cpu_link = sst.Link("cpu_cache_link_" + str(x))
    new_l1_to_cpu_link.connect( (ariel, "cache_link_" + str(x), "50ps"), (l1dict["l1cache_" + str(x)], "upstream0", "50ps") )
    l1links["cpu_cache_link_" + str(x)] = new_l1_to_cpu_link
    new_l1_to_membus = sst.Link("cache_bus_link_" + str(x))
    new_l1_to_membus.connect( (membus, "port" + str(x), "50ps"), (l1dict["l1cache_" + str(x)], "snoop_link", "50ps") )
    l1links["cache_bus_link_" + str(x)] = new_l1_to_membus

memory = sst.Component("memory", "memHierarchy.MemController")
memory.addParams({
	"access_time" : "10ns",
	"mem_size" : "512",
	"clock" : "1GHz",
	"use_dramsim" : "0",
	"device_ini" : "DDR3_micron_32M_8B_x4_sg125.ini",
	"system_ini" : "system.ini",
	})

memory_to_bus_link = sst.Link("mem_bus_link")
memory_to_bus_link.connect( (membus, "port" + str(corecount), "50ps"), (memory, "snoop_link", "50ps") )
