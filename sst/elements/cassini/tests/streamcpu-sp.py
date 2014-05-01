import sst

sst.setProgramOption("timebase", "1ns")

cpu = sst.Component("cpu", "memHierarchy.streamCPU")
cpu.addParams({
		"verbose" : "0",
		"workPerCycle" : "1000",
		"commFreq" : "100",
		"memSize" : "0x1000000",
		"do_write" : "1",
		"num_loadstore" : "100000"
	})

l1cache = sst.Component("l1cache", "memHierarchy.Cache")
l1cache.addParams({
		"cache_frequency" : "1GHz",
		"associativity" : "4",
		"cache_size" : "1MB", 
		"cache_line_size" : "64",
		"num_ways" : "4",
		"num_rows" : "32",
		"blocksize" : "64",
		"access_latency_cycles" : "4",
		"prefetcher" : "cassini.StridePrefetcher",
		"prefetcher:verbose" : "0", 
		"strideprefetcher:reach" : "4",
		"coherence_protocol" : "MESI",
#		"prefetcher" : "cassini.NextBlockPrefetcher",
		"access_time" : "2ns",
		"num_upstream" : "1",
		"printStats" : "1"
	})

membus = sst.Component("membus", "memHierarchy.Bus")
membus.addParams({
		"numPorts" : "2",
		"busDelay" : "20ns"
	})

memory = sst.Component("memory", "memHierarchy.MemController")
memory.addParams({
		"access_time" : "100ns",
		"mem_size" : "512",
		"clock" : "1GHz"
	})

cpu_cache_link = sst.Link("cpu_cache_link")
cpu_cache_link.connect( (cpu, "mem_link", "50ps"), (l1cache, "upstream0", "50ps") )

cache_bus_link = sst.Link("cache_bus_link")
cache_bus_link.connect( (membus, "port0", "50ps"), (l1cache, "snoop_link", "50ps") )

memory_link = sst.Link("mem_bus_link")
memory_link.connect( (membus, "port1", "50ps"), (memory, "snoop_link", "50ps") )
