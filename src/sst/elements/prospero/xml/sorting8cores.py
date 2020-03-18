import sst

sst.setProgramOption("timebase", "1ps")

corecount = 8

prospero={}

for p in range(0, corecount):
	print("Creating prospero component core " + str(p))
        prospero["prospero" + str(p)] = sst.Component("prospero" + str(p), "prospero.prospero")
        prospero["prospero" + str(p)].addParams( {
		"clock" : "1GHz",
		"translateaddresses" : "0",
		"trace" : "/home/sdhammo/subversion/sst-simulator/sst/elements/ariel/pysdl/sort_omp/bsr_omp_algo4-" + str(p) + ".trace",
		"traceformat" : "1",
		"outputlevel" : "0",
		"timemultiplier" : "0.00000000001"
	} )

membus = sst.Component("membus", "memHierarchy.Bus")
membus.addParams( {
	"numPorts" : str(corecount + corecount + 2),
	"busDelay" : "1ns"
	} )

for x in range(0, corecount):
	l1d = sst.Component("l1cache_" + str(x), "memHierarchy.Cache")
	l1d.addParams( {
		"maxL1ResponseTime" : "100000000",
		"num_ways" : "2",
		"num_rows" : "256",
		"blocksize" : "64",
		"access_time" : "1ns",
		"num_upstream" : "1",
		"next_level" : "l2cache_" + str(x),
		"printStats" : "1"
		} )
	prospero_l1d_link = sst.Link("cpu_cache_link_" + str(x))
	prospero_l1d_link.connect( (prospero["prospero" + str(x)], "cache_link", "50ps"), (l1d, "upstream0", "50ps") )
	prospero_l1d_mem_link = sst.Link("l1d_membus_link_" + str(x))
	prospero_l1d_mem_link.connect( (l1d, "snoop_link", "50ps"), (membus, "port" + str(x), "50ps") )

for x in range(0, corecount):
	l2 = sst.Component("l2cache_" + str(x), "memHierarchy.Cache")
	l2.addParams( {
		"num_ways" : "2",
                "num_rows" : "256",
                "blocksize" : "64",
                "access_time" :	"1ns",
#                "num_upstream" : "1",
                "next_level" : "l3cache",
		"printStats" : "1",
		} )
	prospero_l2_link = sst.Link("l2cache_link_" + str(x) )
	prospero_l2_link.connect( (l2, "snoop_link", "50ps"), (membus, "port" + str(corecount + x), "50ps") )

l3 = sst.Component("l3cache", "memHierarchy.Cache")
l3.addParams( {
		"num_ways" : "2",
                "num_rows" : "256",
               	"blocksize" : "64",
               	"access_time" : "1ns",
               	"printStats" : "1",
	} )

l3_membus_link = sst.Link("l3cache_link")
l3_membus_link.connect( (l3, "snoop_link", "50ps"), (membus, "port" + str(corecount + corecount), "50ps") )

memory = sst.Component("fastmemory", "memHierarchy.MemController")
memory.addParams( {
		"access_time" : "70ns",
		"rangeStart" : "0x00000000",
		"mem_size" : "512",
		"clock" : "1GHz",
		"printStats" : "1"
	} )

memory_membus_link = sst.Link("memory_membus_link")
memory_membus_link.connect( (memory, "snoop_link", "50ps"), (membus, "port" + str(corecount + corecount + 1), "50ps") )

print("Done configuring SST model")
