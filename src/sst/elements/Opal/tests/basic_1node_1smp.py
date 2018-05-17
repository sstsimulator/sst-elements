import sst


# Define SST core options
sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stopAtCycle", "0 ns")

# Tell SST what statistics handling we want
sst.setStatisticLoadLevel(4)

clock = "2GHz"

cores = 2*2

#os.environ['OMP_NUM_THREADS'] = str(cores/2)


local_memory_capacity = 128  	# Size of memory in MBs
shared_memory_capacity = 2048	# 2GB
shared_memory = 1
page_size = 4 # In KB 
num_pages = local_memory_capacity * 1024 / page_size + 8*1024*1024/page_size


ariel = sst.Component("cpu", "ariel.ariel")
ariel.addParams({
    "verbose" : 1,
    "clock" : clock,
    "maxcorequeue" : 1024,
    "maxissuepercycle" : 2,
    "maxtranscore": 16,
    "pipetimeout" : 0,
    "corecount" : cores/2,
    "memmgr.memorylevels" : 1,
    "memmgr.pagecount0" : num_pages,
    "memmgr.pagesize0" : page_size * 1024,
    "memmgr.defaultlevel" : 0,
    "arielmode" : 0,
    "appargcount" : 0,
    "max_insts" : 10000,
    "opal_enabled": 1,
    "opal_latency": "30ps",
    "executable" : "./app/opal_test",
    "node" : 0,
})
ariel.enableAllStatistics({"type":"sst.AccumulatorStatistic"})



mmu = sst.Component("mmu", "Samba")
mmu.addParams({
        "os_page_size": 4,
	"perfect": 0,
        "corecount": cores/2,
	"sizes_L1": 3,
	"page_size1_L1": 4,
        "page_size2_L1": 2048,
        "page_size3_L1": 1024*1024,
        "assoc1_L1": 4,
        "size1_L1": 32,
        "assoc2_L1": 4,
        "size2_L1": 32,
        "assoc3_L1": 4,
        "size3_L1": 4,
        "sizes_L2": 4,
        "page_size1_L2": 4,
        "page_size2_L2": 2048,
        "page_size3_L2": 1024*1024,
        "assoc1_L2": 12,
        "size1_L2": 1536,#1536,
        "assoc2_L2": 32, #12,
        "size2_L2": 32, #1536,
        "assoc3_L2": 4,
        "size3_L2": 16,
        "clock": clock,
        "levels": 2,
        "max_width_L1": 3,
        "max_outstanding_L1": 2,
	"max_outstanding_PTWC": 2,
        "latency_L1": 4,
        "parallel_mode_L1": 1,
        "max_outstanding_L2": 2,
        "max_width_L2": 4,
        "latency_L2": 10,
        "parallel_mode_L2": 0,
	"self_connected" : 0,
	"page_walk_latency": 200,
	"size1_PTWC": 32, # this just indicates the number entries of the page table walk cache level 1 (PTEs)
	"assoc1_PTWC": 4, # this just indicates the associtativit the page table walk cache level 1 (PTEs)
	"size2_PTWC": 32, # this just indicates the number entries of the page table walk cache level 2 (PMDs)
	"assoc2_PTWC": 4, # this just indicates the associtativit the page table walk cache level 2 (PMDs)
	"size3_PTWC": 32, # this just indicates the number entries of the page table walk cache level 3 (PUDs)
	"assoc3_PTWC": 4, # this just indicates the associtativit the page table walk cache level 3 (PUDs)
	"size4_PTWC": 32, # this just indicates the number entries of the page table walk cache level 4 (PGD)
	"assoc4_PTWC": 4, # this just indicates the associtativit the page table walk cache level 4 (PGD)
	"latency_PTWC": 10, # This is the latency of checking the page table walk cache
	"opal_latency": "30ps",
	"emulate_faults": 1,
})
mmu.enableAllStatistics({"type":"sst.AccumulatorStatistic"})


opal= sst.Component("opal","Opal")
opal.addParams({
	"clock"				: clock,
	"num_nodes"			: 1,
	"verbose"  			: 1,
	"max_inst" 			: 32,
	"shared_mempools" 		: 1,
	"shared_mem.mempool0.start"	: local_memory_capacity*1024*1024,
	"shared_mem.mempool0.size"	: shared_memory_capacity*1024,
	"shared_mem.mempool0.frame_size": page_size,
	"shared_mem.mempool0.mem_type"	: 0,
	"node0.cores" 			: cores/2,
	"node0.allocation_policy" 	: 0,
	"node0.page_migration" 		: 0,
	"node0.page_migration_policy" 	: 0,
	"node0.num_pages_to_migrate" 	: 0,
	"node0.latency" 		: 2000,
	"node0.memory.start" 		: 0,
	"node0.memory.size" 		: local_memory_capacity*1024,
	"node0.memory.frame_size" 	: page_size,
	"node0.memory.mem_type" 	: 0,
	"num_ports"			: cores,
})
opal.enableAllStatistics({"type":"sst.AccumulatorStatistic"})


l1_params = {
        "cache_frequency": clock,
        "cache_size": "32KiB",
        "associativity": 8,
        "access_latency_cycles": 4,
       	"L1": 1,
        "verbose": 30,
        "maxRequestDelay" : "1000000",
	"shared_memory": shared_memory,
	"node": 0,
}

l2_params = {
        "cache_frequency": clock,
        "cache_size": "256KiB",
        "associativity": 8,
        "access_latency_cycles": 6,
        "mshr_num_entries" : 16,
	"memNIC.network_bw": "96GiB/s",
	"shared_memory": shared_memory,
	"node": 0,
}

l3_params = {
      	"access_latency_cycles" : "12",
      	"cache_frequency" : clock,
      	"associativity" : "16",
      	"cache_size" : "2MB",
      	"mshr_num_entries" : "4096",
      	"memNIC.network_bw": "96GiB/s",
        "num_cache_slices" : 1,
      	"slice_allocation_policy" : "rr",
	"shared_memory": shared_memory,
	"node": 0,
}



class Network:
    def __init__(self, name,networkId,input_latency,output_latency):
        self.name = name
        self.ports = 0
        self.rtr = sst.Component("rtr_%s"%name, "merlin.hr_router")
        self.rtr.addParams({
            "id": networkId,
            "topology": "merlin.singlerouter",
            "link_bw" : "80GiB/s",
            "xbar_bw" : "80GiB/s",
            "flit_size" : "8B",
            "input_latency" : input_latency,
            "output_latency" : output_latency,
            "input_buf_size" : "1KB",
            "output_buf_size" : "1KB",
            })


    def getNextPort(self):
        self.ports += 1
        self.rtr.addParam("num_ports", self.ports)
        return (self.ports-1)



internal_network = Network("internal_network",0,"20ps","20ps")

for next_core in range(cores):

	l1 = sst.Component("l1cache_" + str(next_core), "memHierarchy.Cache")
	l1.addParams(l1_params)

	l2 = sst.Component("l2cache_" + str(next_core), "memHierarchy.Cache")
	l2.addParams(l2_params)

	arielMMULink = sst.Link("cpu_mmu_link_" + str(next_core))
	MMUCacheLink = sst.Link("mmu_cache_link_" + str(next_core))
	PTWMemLink = sst.Link("ptw_mem_link_" + str(next_core))
	PTWOpalLink = sst.Link("ptw_opal_" + str(next_core))
	ArielOpalLink = sst.Link("ariel_opal_" + str(next_core))

	if next_core < cores/2:
        	arielMMULink.connect((ariel, "cache_link_%d"%next_core, "300ps"), (mmu, "cpu_to_mmu%d"%next_core, "300ps"))
		ArielOpalLink.connect((ariel, "opal_link_%d"%next_core, "300ps"), (opal, "requestLink%d"%(2*next_core), "300ps"))
		MMUCacheLink.connect((mmu, "mmu_to_cache%d"%next_core, "300ps"), (l1, "high_network_0", "300ps"))
		PTWOpalLink.connect( (mmu, "ptw_to_opal%d"%next_core, "300ps"), (opal, "requestLink%d"%(2*next_core + 1), "300ps") )
	else:
		PTWMemLink.connect((mmu, "ptw_to_mem%d"%(next_core-cores/2), "300ps"), (l1, "high_network_0", "300ps"))

	l2_core_link = sst.Link("l2cache_" + str(next_core) + "_link")
	l2_core_link.connect((l1, "low_network_0", "300ps"), (l2, "high_network_0", "300ps"))				

	l2_ring_link = sst.Link("l2_ring_link_" + str(next_core))
	l2_ring_link.connect((l2, "cache", "300ps"), (internal_network.rtr, "port%d"%(internal_network.getNextPort()), "300ps"))
			


l3cache = sst.Component("l3cache", "memHierarchy.Cache")
l3cache.addParams(l3_params)
l3cache.addParams({
	"slice_id" : 0,
	"memNIC.addr_range_start": 0,
	"memNIC.addr_range_end": (local_memory_capacity*1024*1024) - 1,
	"memNIC.interleave_size": "0B",
})

l3_ring_link = sst.Link("l3_link")
l3_ring_link.connect( (l3cache, "directory", "300ps"), (internal_network.rtr, "port%d"%(internal_network.getNextPort()), "300ps"))


mem = sst.Component("local_memory", "memHierarchy.MemController")	
mem.addParams({
	"backing" : "none",
	"backend" : "memHierarchy.timingDRAM",
	"backend.id" : 0,
	"backend.addrMapper" : "memHierarchy.roundRobinAddrMapper",
	"backend.addrMapper.interleave_size" : "64B",
	"backend.addrMapper.row_size" : "1KiB",
	"backend.clock" : "1.2GHz",
	"backend.mem_size" : str(local_memory_capacity) + "MiB",
	"backend.channels" : 2,
	"backend.channel.numRanks" : 2,
	"backend.channel.rank.numBanks" : 16,
	"backend.channel.transaction_Q_size" : 32,
	"backend.channel.rank.bank.CL" : 14,
	"backend.channel.rank.bank.CL_WR" : 12,
	"backend.channel.rank.bank.RCD" : 14,
	"backend.channel.rank.bank.TRP" : 14,
	"backend.channel.rank.bank.dataCycles" : 2,
	"backend.channel.rank.bank.pagePolicy" : "memHierarchy.simplePagePolicy",
	"backend.channel.rank.bank.transactionQ" : "memHierarchy.fifoTransactionQ",
	"backend.channel.rank.bank.pagePolicy.close" : 1,
	"shared_memory": 1,
	"node": 0,
})

dc = sst.Component("dc", "memHierarchy.DirectoryController")
dc.addParams({
	"entry_cache_size": 256*1024*1024, #Entry cache size of mem/blocksize
	"clock": "200MHz",
       	"memNIC.network_bw": "96GiB/s",
	"network_address" : 0,
	"memNIC.addr_range_start" : 0,
	"memNIC.addr_range_end" : (local_memory_capacity*1024*1024)-1,
	"memNIC.interleave_size": "0B",
	"shared_memory": shared_memory,
	"node": 0
})

memLink = sst.Link("mem_link")
memLink.connect((mem, "direct_link", "300ps"), (dc, "memory", "300ps"))

netLink = sst.Link("dc_link")
netLink.connect((dc, "network", "300ps"), (internal_network.rtr, "port%d"%(internal_network.getNextPort()), "300ps"))





# External memory configuration

external_network = Network("Ext_Mem_Net",1,"20ns","20ns")
port = external_network.getNextPort()

mem = sst.Component("ExternalNVMmemContr", "memHierarchy.MemController")
mem.addParams({
        "backend.mem_size" : str(shared_memory_capacity) + "MB",
	"backing" : "none",
        "backend" : "memHierarchy.Messier",
        "backendConvertor.backend" : "memHierarchy.Messier",
        "backend.clock" : clock,
        "clock" : clock,
	"node" : 9999, ## does not belong to any node
})

dc = sst.Component("ExtMemDc", "memHierarchy.DirectoryController")
dc.addParams({
	"entry_cache_size": 256*1024*1024, #Entry cache size of mem/blocksize
	"clock": "1GHz",
	"memNIC.network_bw": "80GiB/s",
	"memNIC.addr_range_start" : (local_memory_capacity*1024*1024),
	"memNIC.addr_range_end" : (local_memory_capacity*1024*1024) + (shared_memory_capacity*1024*1024) -1,
	"network_address" : port,
	"node": 9999,
})

messier = sst.Component("ExternalMem" , "Messier")
messier.addParams({
      "clock" : clock,
      "tCL" : 30,
      "tRCD" : 300,
      "tCL_W" : 1000,
      "write_buffer_size" : 32,
      "flush_th" : 90,
      "num_banks" : 16,
      "max_outstanding" : 16,
      "max_writes" : "4",
      "max_current_weight" : 32*50,
      "read_weight" : "5",
      "write_weight" : "5",
      "cacheline_interleaving" : 0,
})

link_nvm_bus_link = sst.Link("External_mem_nvm_link")
link_nvm_bus_link.connect( (messier, "bus", "50ps"), (mem, "cube_link", "50ps") )

memLink = sst.Link("External_mem_dc_link")
memLink.connect( (dc, "memory", "500ps"), (mem, "direct_link", "500ps") )

dcLink = sst.Link("External_mem_link")
dcLink.connect( (dc, "network", "500ps"), (external_network.rtr, "port%d"%port, "500ps") )



# Connecting Internal and External network
def bridge(net0, net1):
    net0port = net0.getNextPort()
    net1port = net1.getNextPort()
    name = "%s-%s"%(net0.name, net1.name)
    bridge = sst.Component("Bridge:%s"%name, "merlin.Bridge")
    bridge.addParams({
        "translator": "memHierarchy.MemNetBridge",
        "network_bw" : "80GiB/s",
    })
    link = sst.Link("B0-%s"%name)
    link.connect( (bridge, "network0", "500ps"), (net0.rtr, "port%d"%net0port, "500ps") )
    link = sst.Link("B1-%s"%name)
    link.connect( (bridge, "network1", "500ps"), (net1.rtr, "port%d"%net1port, "500ps") )


midnet = Network("Bridge",3,"50ps","50ps")
bridge(internal_network, midnet)
bridge(external_network, midnet)


