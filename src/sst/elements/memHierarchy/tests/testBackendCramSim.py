import sst
from mhlib import componentlist


def read_arguments():
    boolUseDefaultConfig = True

def setup_config_params():
    l_params = {}
    if g_boolUseDefaultConfig:
        print("Config file not found... using default configuration")
        l_params = {
            "clockCycle": "1ns",
            "stopAtCycle": "10us",
            "numChannels":"1",
            "numRanksPerChannel":"2",
            "numBankGroupsPerRank":"2",
            "numBanksPerBankGroup":"2",
            "numRowsPerBank":"32768",
            "numColsPerBank":"2048",
            "numBytesPerTransaction":"32",
            "relCommandWidth":"1",
            "readWriteRatio":"1",
            "boolUseReadA":"0",
            "boolUseWriteA":"0",
            "boolUseRefresh":"0",
            "boolAllocateCmdResACT":"0",
            "boolAllocateCmdResREAD":"1",
            "boolAllocateCmdResREADA":"1",
            "boolAllocateCmdResWRITE":"1",
            "boolAllocateCmdResWRITEA":"1",
            "boolAllocateCmdResPRE":"0",
            "boolCmdQueueFindAnyIssuable":"1",
            "boolPrintCmdTrace":"0",
            "strAddressMapStr":"_r_l_R_B_b_h_",
            "bankPolicy":"CLOSE",
            "nRC":"55",
            "nRRD":"4",
            "nRRD_L":"6",
            "nRRD_S":"4",
            "nRCD":"16",
            "nCCD":"4",
            "nCCD_L":"6",
            "nCCD_L_WR":"1",
            "nCCD_S":"4",
            "nAL":"15",
            "nCL":"16",
            "nCWL":"12",
            "nWR":"18",
            "nWTR":"3",
            "nWTR_L":"9",
            "nWTR_S":"3",
            "nRTW":"4",
            "nEWTR":"6",
            "nERTW":"6",
            "nEWTW":"6",
            "nERTR":"6",
            "nRAS":"39",
            "nRTP":"9",
            "nRP":"16",
            "nRFC":"420",
            "nREFI":"9360",
            "nFAW":"16",
            "nBL":"4"
        }
    else:
        l_configFile = open(g_config_file, 'r')
        for l_line in l_configFile:
            l_tokens = l_line.split(' ')
            #print l_tokens[0], ": ", l_tokens[1]
            l_params[l_tokens[0]] = l_tokens[1]

    return l_params

# Command line arguments
g_boolUseDefaultConfig = True
#g_config_file = ""

# Setup global parameters
#[g_boolUseDefaultConfig, g_config_file] = read_arguments()
g_params = setup_config_params()


# Define the simulation components
comp_cpu0 = sst.Component("core0", "memHierarchy.standardCPU")
comp_cpu0.addParams({
    "memFreq" : "100",
    "rngseed" : "101",
    "clock" : "2GHz",
    "memSize" : "1MiB",
    "verbose" : 0,
    "maxOutstanding" : 16,
    "opCount" : 5000,
    "reqsPerIssue" : 4,
    "write_freq" : 40, # 40% writes
    "read_freq" : 60,  # 60% reads
})
iface0 = comp_cpu0.setSubComponent("memory", "memHierarchy.standardInterface")
comp_c0_l1cache = sst.Component("l1cache0.msi", "memHierarchy.Cache")
comp_c0_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : "0"
})
comp_cpu1 = sst.Component("core1", "memHierarchy.standardCPU")
comp_cpu1.addParams({
    "memFreq" : "100",
    "rngseed" : "301",
    "clock" : "2GHz",
    "memSize" : "1MiB",
    "verbose" : 0,
    "maxOutstanding" : 16,
    "opCount" : 5000,
    "reqsPerIssue" : 4,
    "write_freq" : 40, # 40% writes
    "read_freq" : 60,  # 60% reads
})
iface1 = comp_cpu1.setSubComponent("memory", "memHierarchy.standardInterface")
comp_c1_l1cache = sst.Component("l1cache1.msi", "memHierarchy.Cache")
comp_c1_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : "0"
})
comp_n0_bus = sst.Component("n0.bus", "memHierarchy.Bus")
comp_n0_bus.addParams({
      "bus_frequency" : "2 Ghz"
})
comp_n0_l2cache = sst.Component("l2cache0.msi.inclus", "memHierarchy.Cache")
comp_n0_l2cache.addParams({
      "access_latency_cycles" : "20",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "32 KB",
      "debug" : "0"
})
comp_cpu2 = sst.Component("core2", "memHierarchy.standardCPU")
iface2 = comp_cpu2.setSubComponent("memory", "memHierarchy.standardInterface")
comp_cpu2.addParams({
    "memFreq" : "100",
    "rngseed" : "501",
    "clock" : "2GHz",
    "memSize" : "1MiB",
    "verbose" : 0,
    "maxOutstanding" : 16,
    "opCount" : 5000,
    "reqsPerIssue" : 4,
    "write_freq" : 40, # 40% writes
    "read_freq" : 60,  # 60% reads
})
comp_c2_l1cache = sst.Component("l1cache2.msi", "memHierarchy.Cache")
comp_c2_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : "0"
})
comp_cpu3 = sst.Component("core3", "memHierarchy.standardCPU")
iface3 = comp_cpu3.setSubComponent("memory", "memHierarchy.standardInterface")
comp_cpu3.addParams({
    "memFreq" : "100",
    "rngseed" : "701",
    "clock" : "2GHz",
    "memSize" : "1MiB",
    "verbose" : 0,
    "maxOutstanding" : 16,
    "opCount" : 5000,
    "reqsPerIssue" : 4,
    "write_freq" : 40, # 40% writes
    "read_freq" : 60,  # 60% reads
})
comp_c3_l1cache = sst.Component("l1cache3.msi", "memHierarchy.Cache")
comp_c3_l1cache.addParams({
      "access_latency_cycles" : "5",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : "0"
})
comp_n1_bus = sst.Component("n1.bus", "memHierarchy.Bus")
comp_n1_bus.addParams({
      "bus_frequency" : "2 Ghz"
})
comp_n1_l2cache = sst.Component("l2cache1.msi.inclus", "memHierarchy.Cache")
comp_n1_l2cache.addParams({
      "access_latency_cycles" : "20",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "32 KB",
      "debug" : "0"
})
comp_n2_bus = sst.Component("n2.bus", "memHierarchy.Bus")
comp_n2_bus.addParams({
      "bus_frequency" : "2 Ghz"
})
l3cache = sst.Component("l3cache.msi", "memHierarchy.Cache")
l3cache.addParams({
      "access_latency_cycles" : "100",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "16",
      "cache_line_size" : "64",
      "cache_size" : "64 KB",
      "debug" : "0",
      "network_bw" : "25GB/s",
})
comp_chiprtr = sst.Component("chiprtr", "merlin.hr_router")
comp_chiprtr.addParams({
      "xbar_bw" : "1GB/s",
      "link_bw" : "1GB/s",
      "input_buf_size" : "1KB",
      "num_ports" : "2",
      "flit_size" : "72B",
      "output_buf_size" : "1KB",
      "id" : "0",
      "topology" : "merlin.singlerouter"
})
comp_chiprtr.setSubComponent("topology","merlin.singlerouter")
comp_dirctrl = sst.Component("directory.msi", "memHierarchy.DirectoryController")
comp_dirctrl.addParams({
      "coherence_protocol" : "MSI",
      "debug" : "0",
      "entry_cache_size" : "32768",
      "network_bw" : "25GB/s",
      "addr_range_end" : "0x1F000000",
      "addr_range_start" : "0x0"
})
comp_memctrl = sst.Component("memory", "memHierarchy.MemController")
comp_memctrl.addParams({
    "debug" : "0",
    "clock" : "1GHz",
    "request_width" : "64",
    "addr_range_end" : 512*1024*1024-1,
})
comp_memory = comp_memctrl.setSubComponent("backend", "memHierarchy.cramsim")
comp_memory.addParams({
    "access_time" : "2 ns",   # Phy latency
    "mem_size" : "512MiB",
})

# txn gen <--> memHierarchy Bridge
comp_memhBridge = sst.Component("memh_bridge", "cramSim.c_MemhBridge")
comp_memhBridge.addParams(g_params);
comp_memhBridge.addParams({
                     "verbose" : "0",
                     "numTxnPerCycle" : g_params["numChannels"],
                     "strTxnTraceFile" : "arielTrace",
                     "boolPrintTxnTrace" : "1"
                     })
# controller
comp_controller0 = sst.Component("MemController0", "cramSim.c_Controller")
comp_controller0.addParams(g_params)
comp_controller0.addParams({
                "verbose" : "0",
     		"TxnConverter" : "cramSim.c_TxnConverter",
     		"AddrHasher" : "cramSim.c_AddressHasher",
     		"CmdScheduler" : "cramSim.c_CmdScheduler" ,
     		"DeviceController" : "cramSim.c_DeviceController"
     		})


# bank receiver
comp_dimm0 = sst.Component("Dimm0", "cramSim.c_Dimm")
comp_dimm0.addParams(g_params)



# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)


# Define the simulation links
link_c0_l1cache = sst.Link("link_c0_l1cache")
link_c0_l1cache.connect( (iface0, "lowlink", "1000ps"), (comp_c0_l1cache, "highlink", "1000ps") )
link_c0L1cache_bus = sst.Link("link_c0L1cache_bus")
link_c0L1cache_bus.connect( (comp_c0_l1cache, "lowlink", "10000ps"), (comp_n0_bus, "highlink0", "10000ps") )
link_c1_l1cache = sst.Link("link_c1_l1cache")
link_c1_l1cache.connect( (iface1, "lowlink", "1000ps"), (comp_c1_l1cache, "highlink", "1000ps") )
link_c1L1cache_bus = sst.Link("link_c1L1cache_bus")
link_c1L1cache_bus.connect( (comp_c1_l1cache, "lowlink", "10000ps"), (comp_n0_bus, "highlink1", "10000ps") )
link_bus_n0L2cache = sst.Link("link_bus_n0L2cache")
link_bus_n0L2cache.connect( (comp_n0_bus, "lowlink0", "10000ps"), (comp_n0_l2cache, "highlink", "10000ps") )
link_n0L2cache_bus = sst.Link("link_n0L2cache_bus")
link_n0L2cache_bus.connect( (comp_n0_l2cache, "lowlink", "10000ps"), (comp_n2_bus, "highlink0", "10000ps") )
link_c2_l1cache = sst.Link("link_c2_l1cache")
link_c2_l1cache.connect( (iface2, "lowlink", "1000ps"), (comp_c2_l1cache, "highlink", "1000ps") )
link_c2L1cache_bus = sst.Link("link_c2L1cache_bus")
link_c2L1cache_bus.connect( (comp_c2_l1cache, "lowlink", "10000ps"), (comp_n1_bus, "highlink0", "10000ps") )
link_c3_l1cache = sst.Link("link_c3_l1cache")
link_c3_l1cache.connect( (iface3, "lowlink", "1000ps"), (comp_c3_l1cache, "highlink", "1000ps") )
link_c3L1cache_bus = sst.Link("link_c3L1cache_bus")
link_c3L1cache_bus.connect( (comp_c3_l1cache, "lowlink", "10000ps"), (comp_n1_bus, "highlink1", "10000ps") )
link_bus_n1L2cache = sst.Link("link_bus_n1L2cache")
link_bus_n1L2cache.connect( (comp_n1_bus, "lowlink0", "10000ps"), (comp_n1_l2cache, "highlink", "10000ps") )
link_n1L2cache_bus = sst.Link("link_n1L2cache_bus")
link_n1L2cache_bus.connect( (comp_n1_l2cache, "lowlink", "10000ps"), (comp_n2_bus, "highlink1", "10000ps") )
link_bus_l3cache = sst.Link("link_bus_l3cache")
link_bus_l3cache.connect( (comp_n2_bus, "lowlink0", "10000ps"), (l3cache, "highlink", "10000ps") )
link_cache_net_0 = sst.Link("link_cache_net_0")
link_cache_net_0.connect( (l3cache, "directory", "10000ps"), (comp_chiprtr, "port1", "2000ps") )
link_dir_net_0 = sst.Link("link_dir_net_0")
link_dir_net_0.connect( (comp_chiprtr, "port0", "2000ps"), (comp_dirctrl, "network", "2000ps") )
link_dir_mem_link = sst.Link("link_dir_mem_link")
link_dir_mem_link.connect( (comp_dirctrl, "memory", "10000ps"), (comp_memctrl, "highlink", "10000ps") )


link_dir_cramsim_link = sst.Link("link_dir_cramsim_link")
link_dir_cramsim_link.connect( (comp_memory, "cramsim_link", "2ns"), (comp_memhBridge, "cpuLink", "2ns") )

# memhBridge(=TxnGen) <-> Memory Controller 
memHLink = sst.Link("memHLink_1")
memHLink.connect( (comp_memhBridge, "memLink", g_params["clockCycle"]), (comp_controller0, "txngenLink", g_params["clockCycle"]) )

# Controller <-> Dimm
cmdLink = sst.Link("cmdLink_1")
cmdLink.connect( (comp_controller0, "memLink", g_params["clockCycle"]), (comp_dimm0, "ctrlLink", g_params["clockCycle"]) )

