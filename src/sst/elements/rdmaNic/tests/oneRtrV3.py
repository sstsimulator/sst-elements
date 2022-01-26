import os
import sst

coherence_protocol="MESI"
#coherence_protocol="MSI"

#debug_addr=0x7ffffb88
debug_addr=0x7ffffb80

#debug_addr=0x4001c0

l2_debug = 1 
dc_debug = 1 
mc_debug = 1 

nicCache_debug = 0 
stdMem_debug = 0 

debug_start_time =  26760650595 
#debug_start_time = 0 

import vanadisBlock as cpuBlock

cpu_clock = os.getenv("VANADIS_CPU_CLOCK", "2.3GHz")

class Mem_Builder:
	def __init__(self):
		pass

	def build( self, nodeId ):

		prefix = 'node' + str(nodeId) 

		cpu0_l2cache = sst.Component(prefix + ".l2cache", "memHierarchy.Cache")
		cpu0_l2cache.addParams({
			"access_latency_cycles" : "14",
			"cache_frequency" : cpu_clock,
			"replacement_policy" : "lru",
			"coherence_protocol" : coherence_protocol,
			"associativity" : "16",
			"cache_line_size" : "64",
			"cache_size" : "1MB",
			"debug": l2_debug,
			"debug_mask" : 1<<3 | 1<<5,
			"debug_level" : "10",
			"debug_start_time": debug_start_time,
			"debug_addr" : debug_addr,
		})
		l2cache_2_l1caches = cpu0_l2cache.setSubComponent("cpulink", "memHierarchy.MemLink")
		l2cache_2_mem = cpu0_l2cache.setSubComponent("memlink", "memHierarchy.MemNIC")

		l2cache_2_mem.addParams({
			"group" : 1,
			"destinations" : "2,3", # DC, SHMEMNIC
			"network_bw" : "25GB/s",
			"debug": 0,
			"debug_level": 10,
		})

		comp_chiprtr = sst.Component(prefix + ".chiprtr", "merlin.hr_router")
		comp_chiprtr.addParams({
			  "xbar_bw" : "50GB/s",
			  "link_bw" : "25GB/s",
			  "input_buf_size" : "40KB",
			  "output_buf_size" : "40KB",
#			  "xbar_bw" : "1GB/s",
#			  "link_bw" : "1GB/s",
#			  "input_buf_size" : "2KB",
#			  "output_buf_size" : "2KB",
			  "num_ports" : "4",
			  "flit_size" : "72B",
			  "id" : "0",
			  "topology" : "merlin.singlerouter"
		})
		comp_chiprtr.setSubComponent("topology","merlin.singlerouter")

		dirctrl = sst.Component(prefix + ".dirctrl", "memHierarchy.DirectoryController")
		dirctrl.addParams({
			"coherence_protocol" : coherence_protocol,
			"entry_cache_size" : "1024",
			"addr_range_start" : "0x0",
			"addr_range_end" : "0x7fffffff",
			"debug": dc_debug,
			"debug_start_time": debug_start_time,
			"debug_mask" : 1<<5,
			"debug_level" : 10,
			"debug_addr" : debug_addr,
		})
		dirtoM = dirctrl.setSubComponent("memlink", "memHierarchy.MemLink")
		dirNIC = dirctrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
		dirNIC.addParams({
			  "network_bw" : "25GB/s",
			  "group" : 2,
			  "sources": "1,3", # L2, SHMEMNIC
		})

		memctrl = sst.Component(prefix + ".memory", "memHierarchy.MemController")
		memctrl.addParams({
			"clock" : cpu_clock,
			"backend.mem_size" : "4GiB",
			"backing" : "malloc",
           	"addr_range_start" : "0x0",
			"addr_range_end" : "0x7fffffff",
			"debug" : mc_debug,
			"debug_mask" : 1<<3 | 1<<4 | 1<<10 | 1<<11,
			"debug_level" : 10,
			"debug_start_time": debug_start_time,
			"debug_addr" : debug_addr,
		})
		memToDir = memctrl.setSubComponent("cpulink", "memHierarchy.MemLink")

		memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
		memory.addParams({
			  "mem_size" : "2GiB",
			  "access_time" : "1 ns",
			  "debug" : 0,
			  "debug_level" : 10,
		})

		link_l2cache_rtr = sst.Link(prefix + ".link_l2cache_rtr")
		link_l2cache_rtr.connect( (l2cache_2_mem, "port", "1ns"), (comp_chiprtr, "port0", "1ns") )

		link_dir_rtr = sst.Link(prefix + ".link_dir_rtr")
		link_dir_rtr.connect( (comp_chiprtr, "port1", "1ns"), (dirNIC, "port", "1ns") )

		link_dir_mem = sst.Link(prefix + ".link_dir_mem")
		link_dir_mem.connect( (dirtoM, "port", "1ns"), (memToDir, "port", "1ns") )

		return (l2cache_2_l1caches, "port", "1ns"), (comp_chiprtr, "port2", "1ns"), (comp_chiprtr, "port3", "1ns")

class Nic_Builder:
	def __init__(self,numNodes):
		self.numNodes = numNodes

	def build( self, nodeId ):

		print("nodeId {}".format(nodeId ))
		prefix = 'node' + str(nodeId)
		print(prefix)
		nic = sst.Component( prefix + ".nic", "rdmaNic.nic")
		nic.addParams({
		#		"clock" : "8GHz",
				"clock" : "1GHz",
				"debug":0,
				"debug_level": 0,
				"useDmaCache": "true",
				#"debug_mask": 1<<1 | 1 << 2,
				#"debug_mask": 1<<1,
				"debug_mask": -1,
				"maxPendingCmds" : 128,
				"maxMemReqs" : 256,
				"maxCmdQSize" : 128,
				"cache_line_size"    : 64,
			#    "addr_range_start" : 0,
			#    "addr_range_end" : 0x7fffffff,
				'baseAddr': 0x80000000,
				'cmdQSize' : 64,
			})
		nic.addParam( 'nicId', nodeId )
		nic.addParam( 'pesPerNode', 1 )
		nic.addParam( 'numNodes', self.numNodes )


		# NIC DMA interface
		dmaIf = nic.setSubComponent("dma", "memHierarchy.standardInterface")
		dmaIf.addParams({
              "debug" : stdMem_debug,
              "debug_level" : 10,
              "debug_start_time": debug_start_time,
			"debug_addr" : debug_addr,
		})

		# NIC MMIO interface
		mmioIf = nic.setSubComponent("mmio", "memHierarchy.standardInterface")
		mmioIf.addParams({
              "debug" : stdMem_debug,
              "debug_level" : 10,
              "debug_start_time": debug_start_time,
			"debug_addr" : debug_addr,
		})

		# NIC DMA Cache
		dmaCache = sst.Component(prefix + ".nicDmaCache", "memHierarchy.Cache")
		dmaCache.addParams({
              "access_latency_cycles" : "1",
#              "access_latency_cycles" : "2",
              "cache_frequency" : cpu_clock,
              "replacement_policy" : "lru",
              "coherence_protocol" : coherence_protocol,
              "associativity" : "8",
              #"associativity" : "16",
              "cache_line_size" : "64",
              #"cache_size" : "8MB",
              "cache_size" : "32KB",
              "L1" : "1",
			"debug": nicCache_debug,
			"debug_start_time": debug_start_time,
              "debug_level" : 10
        })

		# Cache to CPU interface
		dmaCacheToCpu = dmaCache.setSubComponent("cpulink", "memHierarchy.MemLink")

		# Cache to memory interface
		dmaCacheToMem = dmaCache.setSubComponent("memlink", "memHierarchy.MemNIC")
		dmaCacheToMem.addParams({
			"group" : 1,
			"destinations" : "2,3", # DC
			"network_bw" : "25GB/s",
			"debug": 0,
			"debug_level": 10,
		})

		# CPU to cache link
		dmaCacheToCpu_link = sst.Link(prefix + ".dmaCacheToCpu_link")
		dmaCacheToCpu_link.connect( (dmaIf, "port", "1ns"), (dmaCacheToCpu, "port", "1ns") )

		# NIC MMIO interface to memNIC 
		mmioNIC = mmioIf.setSubComponent("memlink", "memHierarchy.MemNIC")
		mmioNIC.addParams({
			"group" : 3,
			"sources" : "1", # L2
			"destinations" : "2", # DC
			"network_bw" : "25GB/s",
			"debug": 0,
			"debug_level": 10,
		})

		# NIC internode interface 
		netLink = nic.setSubComponent( "rtrLink", "merlin.linkcontrol" )
		netLink.addParam("link_bw","16GB/s")
		netLink.addParam("input_buf_size","14KB")
		netLink.addParam("output_buf_size","14KB")

		return (dmaCacheToMem,"port","10000ps"), (mmioNIC, "port", "10000ps"), (netLink,"rtr_port", '10ns')

class Endpoint():
    def __init__(self,numNodes):
        self.numNodes = numNodes

    def prepParams(self):
        pass

    def build(self, nodeId, extraKeys ):
        print( "Endpoint.build", nodeId )

        prefix = 'node' + str(nodeId);

        cpuBuilder = cpuBlock.Vanadis_Builder()
        memBuilder = Mem_Builder()
        nicBuilder = Nic_Builder(self.numNodes)

        cpu = cpuBuilder.build(nodeId,0)
        L2_port, dmaNic_port, mmioNic_port = memBuilder.build(nodeId)
        dmaNic, mmioNic, netLink = nicBuilder.build(nodeId)

        link_cpu_L2 = sst.Link(prefix + ".link_cpu_L2")
        link_cpu_L2.connect( cpu, L2_port) 

        link_nic_mmio = sst.Link( prefix + ".link_nic_mmio")
        link_nic_mmio.connect( mmioNic, mmioNic_port)

        link_nic_dma = sst.Link( prefix + ".link_nic_dma")
        link_nic_dma.connect( dmaNic, dmaNic_port)
        return netLink

