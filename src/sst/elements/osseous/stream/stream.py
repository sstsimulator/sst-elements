import sst
import os

class RtrPorts:
    def __init__(self):
        self._next_addr = 0 
    def nextPort(self):
        res = self._next_addr
        self._next_addr = self._next_addr + 1
        return res
    def numPorts(self):
        return self._next_addr

sst.setProgramOption("timebase", "1ns")

sst_root = os.getenv( "SST_ROOT" )
rtrInfo = RtrPorts()

pagesize = 4096
memoryperlevel = 2048

noc = sst.Component("noc", "merlin.hr_router")
noc.addParams({
	"id" : 0,
	"topology" : "merlin.singlerouter",
	"link_bw" : "320GB/s",
	"xbar_bw" : "512GB/s",
	"input_latency" : "4ns",
	"output_latency" : "4ns",
	"input_buf_size" : "4KiB",
	"output_buf_size" : "4KiB",
	"flit_size" : "72B"
	})

corecount = 4;

ariel = sst.Component("a0", "ariel.ariel")
ariel.addParams({
        "verbose" : "1",
	"clock" : "2GHz",
        "maxcorequeue" : "256",
        "maxissuepercycle" : "2",
        "pipetimeout" : "0",
	"corecount" : corecount,
        "executable" : "/home/sdhammo/subversion/sst-simulator-org-trunk/tools/ariel/femlm/examples/stream/mlmstream",
        "arielmode" : "1",
        "arieltool" : "/home/sdhammo/subversion/sst-simulator-org-trunk/tools/ariel/femlm/femlmtool.so",
        "memorylevels" : "2",
	"pagecount0" : (memoryperlevel * 1024 * 1024) / pagesize,
	"pagecount1" : (memoryperlevel * 1024 * 1024) / pagesize,
        "defaultlevel" : os.getenv("ARIEL_OVERRIDE_POOL", 1)
        })

for x in range(corecount) :
	l1cache = sst.Component("l1cache" + str(x), "memHierarchy.Cache")
	l1cache.addParams({
        	"cache_frequency" : "2GHz",
	        "cache_size" : "16 KB",
        	"coherence_protocol" : "MESI",
	        "replacement_policy" : "lru",
	        "associativity" : "2",
	        "access_latency_cycles" : "2",
	        "low_network_links" : "1",
	        "cache_line_size" : "64",
	        "L1" : "1",
	        "debug" : "0",
	        "statistics" : "1"
		})

	l2cache_net_addr = rtrInfo.nextPort()
	l2cache = sst.Component("l2cache" + str(x), "memHierarchy.Cache")
	l2cache.addParams({
        	"cache_frequency" : "1500MHz",
	        "cache_size" : "64 KB",
        	"coherence_protocol" : "MESI",
	        "replacement_policy" : "lru",
	        "associativity" : "16",
	        "access_latency_cycles" : "10",
	        "low_network_links" : "1",
	        "cache_line_size" : "64",
		"directory_at_next_level" : 1,
	        "L1" : "0",
	        "debug" : "0",
	        "statistics" : "1",
		"network_address" : l2cache_net_addr
		})
	ariel_core_link = sst.Link("cpu_cache_link_" + str(x))
	ariel_core_link.connect( (ariel, "cache_link_" + str(x), "50ps"), (l1cache, "high_network_0", "50ps") )
	l1tol2_link = sst.Link("l1_l2_link_" + str(x))
	l1tol2_link.connect( (l2cache, "high_network_0", "50ps"), (l1cache, "low_network_0", "50ps") )
	l2cache_link = sst.Link("l2cache_link_" + str(x))
	l2cache_link.connect( (l2cache, "directory", "50ps"), (noc, "port" + str(l2cache_net_addr), "50ps") )
	
fast_memory = sst.Component("fast_memory", "memHierarchy.MemController")
fast_memory.addParams({
        "coherence_protocol" : "MESI",
        "access_time" : "60ns",
        "mem_size" : memoryperlevel,
        "rangeStart" : 0,
	"rangeEnd" : memoryperlevel * 1024 * 1024,
        "clock" : "2GHz",
        "use_dramsim" : "0",
        "device_ini" : "DDR3_micron_32M_8B_x4_sg125.ini",
        "system_ini" : "system.ini",
	"statistics" : 1
        })

fast_dc_port = rtrInfo.nextPort()
fast_dc = sst.Component("fast_dc", "memHierarchy.DirectoryController")
fast_dc.addParams({
	"coherence_protocol" : "MESI",
	"network_bw" : "320GB/s",
	"addr_range_start" : "0",
	"addr_range_end" : memoryperlevel * 1024 * 1024,
	"entry_cache_size" : 128 * 1024,
	"clock" : "1GHz",
	"statistics" : 1,
	"network_address" : fast_dc_port
	})

fast_dc_link = sst.Link("fast_dc_link")
fast_dc_link.connect( (fast_memory, "direct_link", "50ps") , (fast_dc, "memory", "50ps") )
fast_net_link = sst.Link("fast_dc_net_link")
fast_net_link.connect( (fast_dc, "network", "50ps") , (noc, "port" + str(fast_dc_port), "50ps") )

slow_memory = sst.Component("slow_memory", "memHierarchy.MemController")
slow_memory.addParams({
        "coherence_protocol" : "MESI",
        "access_time" : "60ns",
        "mem_size" : memoryperlevel,
        "rangeStart" : 0,
	"rangeEnd" : memoryperlevel * 1024 * 1024,
        "clock" : "300MHz",
        "use_dramsim" : "0",
        "device_ini" : "DDR3_micron_32M_8B_x4_sg125.ini",
        "system_ini" : "system.ini",
	"statistics" : 1
        })

slow_dc_port = rtrInfo.nextPort()
slow_dc = sst.Component("slow_dc", "memHierarchy.DirectoryController")
slow_dc.addParams({
	"coherence_protocol" : "MESI",
	"network_bw" : "50GB/s",
	"addr_range_start" : memoryperlevel * 1024 * 1024,
	"addr_range_end" : memoryperlevel * 1024 * 1024 * 2,
	"entry_cache_size" : 128 * 1024,
	"clock" : "512MHz",
	"statistics" : 1,
	"network_address" : slow_dc_port
	})

slow_dc_link = sst.Link("slow_dc_link")
slow_dc_link.connect( (slow_memory, "direct_link", "50ps") , (slow_dc, "memory", "50ps") )
slow_net_link = sst.Link("slow_dc_net_link")
slow_net_link.connect( (slow_dc, "network", "50ps") , (noc, "port" + str(slow_dc_port), "50ps") )

cpu_cache_link = sst.Link("cpu_cache_link")
cpu_cache_link.connect( (ariel, "cache_link_0", "50ps"), (l1cache, "high_network_0", "50ps") )

noc.addParam("num_ports", rtrInfo.numPorts())
print "Finished Configuration of SST"
