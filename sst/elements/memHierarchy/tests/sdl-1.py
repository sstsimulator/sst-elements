# SDL-1:  Simple MemHierarchy test

import sst

memDebug = 0
coherency = "MSI"
lat = "1ns"
buslat = "50ps"

sst.setProgramOption("timebase", "1ns")

cpu = sst.Component("cpu", "memHierarchy.trivialCPU")
cpu.addParams({"workPerCycle": 1000, "commFreq": 100, "memSize": 0x1000, "do_write": 1, "num_loadstore": 1000})

l1cache = sst.Component("l1cache", "memHierarchy.Cache")
l1cache.addParams({
	"cache_frequency": "2 GHz",
	"cache_size": "2 KB",
	"associativity": 4,
	"cache_line_size" : 64,
	"coherence_protocol": coherency,
	"replacement_policy": "lru",
	"access_latency_cycles" : 4,
	"low_network_links": 1,
	"L1" : 1,
	"debug" : memDebug,
	"statistics" : 1})

memory = sst.Component("memory", "memHierarchy.MemController")
memory.addParams({
	"access_time" : "1000 ns",
	"mem_size" : 512,
	"coherence_protocol": coherency,
	"clock" : "1 GHz",
	"debug" : memDebug } )

cpuLink = sst.Link("cpu_cache_link")
cpuLink.connect((cpu, "mem_link", lat), (l1cache, "high_network_0", lat))

link = sst.Link("cache_mem_link")
link.connect((l1cache, "low_network_0", buslat), (memory, "direct_link", buslat))
