# SDL-1:  Simple MemHierarchy test

import sst

memDebug = 0
busLat = "50 ps"
lat = "1ns"

sst.setProgramOption("timebase", "1ns")

cpu = sst.Component("cpu", "memHierarchy.trivialCPU")
cpu.addParams({"workPerCycle": 1000, "commFreq": 100, "memSize": 0x1000, "do_write": 1, "num_loadstore": 1000})

l1cache = sst.Component("l1cache", "memHierarchy.Cache")
l1cache.addParams({
	"num_ways": 4,
	"num_rows": 16,
	"blocksize" : 32,
	"access_time" : "2 ns",
	"num_upstream" : 1,
	"debug" : memDebug,
	"printStats" : 1})

membus = sst.Component("membus", "memHierarchy.Bus")
membus.addParams({
	"numPorts": 2,
	"busDelay": "20 ns",
	"atomicDelivery" : 1,
	"debug" : memDebug })

memory = sst.Component("memory", "memHierarchy.MemController")
memory.addParams({
	"access_time" : "1000 ns",
	"mem_size" : 512,
	"clock" : "1 GHz",
	"debug" : memDebug } )

cacheBusLink = sst.Link("mem_bus_link")
cacheBusLink.connect((membus, "port0", busLat), (l1cache, "snoop_link", busLat))

memBusLink = sst.Link("cache_bus_link")
memBusLink.connect((membus, "port1", busLat), (memory, "snoop_link", busLat))

cpuCacheLink = sst.Link("cpu_cache_link")
cpu.addLink(cpuCacheLink, "mem_link", lat)
l1cache.addLink(cpuCacheLink, "upstream0", lat)

