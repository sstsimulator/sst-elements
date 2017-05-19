# Automatically generated SST Python input
import sst


def read_arguments():
    boolUseDefaultConfig = True
#    config_file = getcwd()+"/ddr4_verimem.cfg"
"""
    for arg in sys.argv:
        if arg.find("--configfile=") != -1:
            substrIndex = arg.find("=")+1
            config_file = arg[substrIndex:]
            boolUseDefaultConfig = False
            print "Config file:", config_file
    return [boolUseDefaultConfig, config_file]
"""
def setup_config_params():
    l_params = {}
    if g_boolUseDefaultConfig:
        print "Config file not found... using default configuration"
        l_params = {
            "clockCycle": "1ns",
            "stopAtCycle": "10us",
            "numTxnGenReqQEntries":"""50""",
            "numTxnGenResQEntries":"""50""",
            "numTxnUnitReqQEntries":"""50""",
            "numTxnUnitResQEntries":"""50""",
            "numCmdReqQEntries":"""400""",
            "numCmdResQEntries":"""400""",
            "numChannelsPerDimm":"""1""",
            "numRanksPerChannel":"""2""",
            "numBankGroupsPerRank":"""2""",
            "numBanksPerBankGroup":"""2""",
            "numRowsPerBank":"""32768""",
            "numColsPerBank":"""2048""",
            "numBytesPerTransaction":"""32""",
            "relCommandWidth":"""1""",
            "readWriteRatio":"""1""",
            "boolUseReadA":"""0""",
            "boolUseWriteA":"""0""",
            "boolUseRefresh":"""0""",
            "boolAllocateCmdResACT":"""0""",
            "boolAllocateCmdResREAD":"""1""",
            "boolAllocateCmdResREADA":"""1""",
            "boolAllocateCmdResWRITE":"""1""",
            "boolAllocateCmdResWRITEA":"""1""",
            "boolAllocateCmdResPRE":"""0""",
            "boolCmdQueueFindAnyIssuable":"""1""",
            "boolPrintCmdTrace":"""0""",
            "strAddressMapStr":"""_r_l_R_B_b_h_""",
            "bankPolicy":"""0""",
            "nRC":"""55""",
            "nRRD":"""4""",
            "nRRD_L":"""6""",
            "nRRD_S":"""4""",
            "nRCD":"""16""",
            "nCCD":"""4""",
            "nCCD_L":"""6""",
            "nCCD_L_WR":"""1""",
            "nCCD_S":"""4""",
            "nAL":"""15""",
            "nCL":"""16""",
            "nCWL":"""12""",
            "nWR":"""18""",
            "nWTR":"""3""",
            "nWTR_L":"""9""",
            "nWTR_S":"""3""",
            "nRTW":"""4""",
            "nEWTR":"""6""",
            "nERTW":"""6""",
            "nEWTW":"""6""",
            "nERTR":"""6""",
            "nRAS":"""39""",
            "nRTP":"""9""",
            "nRP":"""16""",
            "nRFC":"""420""",
            "nREFI":"""9360""",
            "nFAW":"""16""",
            "nBL":"""4"""
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


# Define SST core options
sst.setProgramOption("timebase", "1ps")
#sst.setProgramOption("stopAtCycle", "11000us")


# Define the simulation components
comp_cpu0 = sst.Component("cpu0", "memHierarchy.trivialCPU")
comp_cpu0.addParams({
      "commFreq" : "100",
      "rngseed" : "101",
      "do_write" : "1",
      "num_loadstore" : "1000",
      "memSize" : "0x100000",
})
comp_c0_l1cache = sst.Component("c0.l1cache", "memHierarchy.Cache")
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
comp_cpu1 = sst.Component("cpu1", "memHierarchy.trivialCPU")
comp_cpu1.addParams({
      "commFreq" : "100",
      "rngseed" : "301",
      "do_write" : "1",
      "num_loadstore" : "1000",
      "memSize" : "0x100000",
})
comp_c1_l1cache = sst.Component("c1.l1cache", "memHierarchy.Cache")
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
comp_n0_l2cache = sst.Component("n0.l2cache", "memHierarchy.Cache")
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
comp_cpu2 = sst.Component("cpu2", "memHierarchy.trivialCPU")
comp_cpu2.addParams({
      "commFreq" : "100",
      "rngseed" : "501",
      "do_write" : "1",
      "num_loadstore" : "1000",
      "memSize" : "0x100000",
})
comp_c2_l1cache = sst.Component("c2.l1cache", "memHierarchy.Cache")
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
comp_cpu3 = sst.Component("cpu3", "memHierarchy.trivialCPU")
comp_cpu3.addParams({
      "commFreq" : "100",
      "rngseed" : "701",
      "do_write" : "1",
      "num_loadstore" : "1000",
      "memSize" : "0x100000",
})
comp_c3_l1cache = sst.Component("c3.l1cache", "memHierarchy.Cache")
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
comp_n1_l2cache = sst.Component("n1.l2cache", "memHierarchy.Cache")
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
comp_l3cache = sst.Component("l3cache", "memHierarchy.Cache")
comp_l3cache.addParams({
      "access_latency_cycles" : "100",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "16",
      "cache_line_size" : "64",
      "cache_size" : "64 KB",
      "debug" : "0",
      "network_address" : "1",
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
comp_dirctrl = sst.Component("dirctrl", "memHierarchy.DirectoryController")
comp_dirctrl.addParams({
      "coherence_protocol" : "MSI",
      "debug" : "0",
      "network_address" : "0",
      "entry_cache_size" : "32768",
      "network_bw" : "25GB/s",
      "addr_range_end" : "0x1F000000",
      "addr_range_start" : "0x0"
})
comp_memory = sst.Component("memory", "memHierarchy.MemController")
comp_memory.addParams({
      "coherence_protocol" : "MSI",
      "debug" : "0",
      "backend" : "memHierarchy.cramsim",
      "backend.access_time" : "2 ns",   # Phy latency
      "backend.mem_size" : "512MiB",
      "clock" : "1GHz",
      "request_width" : "64"
})

# address hasher
comp_addressHasher = sst.Component("AddrHash0", "CramSim.c_AddressHasher")
comp_addressHasher.addParams(g_params)


# txn gen --> memHierarchy Bridge
comp_memhBridge = sst.Component("memh_bridge", "CramSim.c_MemhBridge")
comp_memhBridge.addParams(g_params);

# txn unit
comp_txnUnit0 = sst.Component("TxnUnit0", "CramSim.c_TxnUnit")
comp_txnUnit0.addParams(g_params)

# cmd unit
comp_cmdUnit0 = sst.Component("CmdUnit0", "CramSim.c_CmdUnit")
comp_cmdUnit0.addParams(g_params)

# bank receiver
comp_dimm0 = sst.Component("Dimm0", "CramSim.c_Dimm")
comp_dimm0.addParams(g_params)



# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
sst.enableAllStatisticsForComponentType("memHierarchy.Cache")
sst.enableAllStatisticsForComponentType("memHierarchy.MemController")
sst.enableAllStatisticsForComponentType("memHierarchy.DirectoryController")


# Define the simulation links
link_c0_l1cache = sst.Link("link_c0_l1cache")
link_c0_l1cache.connect( (comp_cpu0, "mem_link", "1000ps"), (comp_c0_l1cache, "high_network_0", "1000ps") )
link_c0L1cache_bus = sst.Link("link_c0L1cache_bus")
link_c0L1cache_bus.connect( (comp_c0_l1cache, "low_network_0", "10000ps"), (comp_n0_bus, "high_network_0", "10000ps") )
link_c1_l1cache = sst.Link("link_c1_l1cache")
link_c1_l1cache.connect( (comp_cpu1, "mem_link", "1000ps"), (comp_c1_l1cache, "high_network_0", "1000ps") )
link_c1L1cache_bus = sst.Link("link_c1L1cache_bus")
link_c1L1cache_bus.connect( (comp_c1_l1cache, "low_network_0", "10000ps"), (comp_n0_bus, "high_network_1", "10000ps") )
link_bus_n0L2cache = sst.Link("link_bus_n0L2cache")
link_bus_n0L2cache.connect( (comp_n0_bus, "low_network_0", "10000ps"), (comp_n0_l2cache, "high_network_0", "10000ps") )
link_n0L2cache_bus = sst.Link("link_n0L2cache_bus")
link_n0L2cache_bus.connect( (comp_n0_l2cache, "low_network_0", "10000ps"), (comp_n2_bus, "high_network_0", "10000ps") )
link_c2_l1cache = sst.Link("link_c2_l1cache")
link_c2_l1cache.connect( (comp_cpu2, "mem_link", "1000ps"), (comp_c2_l1cache, "high_network_0", "1000ps") )
link_c2L1cache_bus = sst.Link("link_c2L1cache_bus")
link_c2L1cache_bus.connect( (comp_c2_l1cache, "low_network_0", "10000ps"), (comp_n1_bus, "high_network_0", "10000ps") )
link_c3_l1cache = sst.Link("link_c3_l1cache")
link_c3_l1cache.connect( (comp_cpu3, "mem_link", "1000ps"), (comp_c3_l1cache, "high_network_0", "1000ps") )
link_c3L1cache_bus = sst.Link("link_c3L1cache_bus")
link_c3L1cache_bus.connect( (comp_c3_l1cache, "low_network_0", "10000ps"), (comp_n1_bus, "high_network_1", "10000ps") )
link_bus_n1L2cache = sst.Link("link_bus_n1L2cache")
link_bus_n1L2cache.connect( (comp_n1_bus, "low_network_0", "10000ps"), (comp_n1_l2cache, "high_network_0", "10000ps") )
link_n1L2cache_bus = sst.Link("link_n1L2cache_bus")
link_n1L2cache_bus.connect( (comp_n1_l2cache, "low_network_0", "10000ps"), (comp_n2_bus, "high_network_1", "10000ps") )
link_bus_l3cache = sst.Link("link_bus_l3cache")
link_bus_l3cache.connect( (comp_n2_bus, "low_network_0", "10000ps"), (comp_l3cache, "high_network_0", "10000ps") )
link_cache_net_0 = sst.Link("link_cache_net_0")
link_cache_net_0.connect( (comp_l3cache, "directory", "10000ps"), (comp_chiprtr, "port1", "2000ps") )
link_dir_net_0 = sst.Link("link_dir_net_0")
link_dir_net_0.connect( (comp_chiprtr, "port0", "2000ps"), (comp_dirctrl, "network", "2000ps") )
link_dir_mem_link = sst.Link("link_dir_mem_link")
link_dir_mem_link.connect( (comp_dirctrl, "memory", "10000ps"), (comp_memory, "direct_link", "10000ps") )

link_dir_cramsim_link = sst.Link("link_dir_cramsim_link")
link_dir_cramsim_link.connect( (comp_memory, "cube_link", "2ns"), (comp_memhBridge, "linkCPU", "2ns") )

txnReqLink_0 = sst.Link("txnReqLink_0")
txnReqLink_0.connect( (comp_memhBridge, "outTxnGenReqPtr", g_params["clockCycle"]), (comp_txnUnit0, "inTxnGenReqPtr", g_params["clockCycle"]) )

# memhBridge(=TxnGen) <- TxnUnit (Req)(Token)
txnTokenLink_0 = sst.Link("txnTokenLink_0")
txnTokenLink_0.connect( (comp_memhBridge, "inTxnUnitReqQTokenChg", g_params["clockCycle"]), (comp_txnUnit0, "outTxnGenReqQTokenChg", g_params["clockCycle"]) )

# memhBridge(=TxnGen) <- TxnUnit (Res)(Txn)
txnResLink_0 = sst.Link("txnResLink_0")
txnResLink_0.connect( (comp_memhBridge, "inTxnUnitResPtr", g_params["clockCycle"]), (comp_txnUnit0, "outTxnGenResPtr", g_params["clockCycle"]) )

# memhBridge(=TxnGen) -> TxnUnit (Res)(Token)
txnTokenLink_1 = sst.Link("txnTokenLink_1")
txnTokenLink_1.connect( (comp_memhBridge, "outTxnGenResQTokenChg", g_params["clockCycle"]), (comp_txnUnit0, "inTxnGenResQTokenChg", g_params["clockCycle"]) )



# TXNUNIT / CMDUNIT LINKS
# TxnUnit -> CmdUnit (Req) (Cmd)
cmdReqLink_0 = sst.Link("cmdReqLink_0")
cmdReqLink_0.connect( (comp_txnUnit0, "outCmdUnitReqPtrPkg", g_params["clockCycle"]), (comp_cmdUnit0, "inTxnUnitReqPtr", g_params["clockCycle"]) )

# TxnUnit <- CmdUnit (Req) (Token)
cmdTokenLink_0 = sst.Link("cmdTokenLink_0")
cmdTokenLink_0.connect( (comp_txnUnit0, "inCmdUnitReqQTokenChg", g_params["clockCycle"]), (comp_cmdUnit0, "outTxnUnitReqQTokenChg", g_params["clockCycle"]) )

# TxnUnit <- CmdUnit (Res) (Cmd)
cmdResLink_0 = sst.Link("cmdResLink_0")
cmdResLink_0.connect( (comp_txnUnit0, "inCmdUnitResPtr", g_params["clockCycle"]), (comp_cmdUnit0, "outTxnUnitResPtr", g_params["clockCycle"]) )




# CMDUNIT / DIMM LINKS
# CmdUnit -> Dimm (Req) (Cmd)
cmdReqLink_1 = sst.Link("cmdReqLink_1")
cmdReqLink_1.connect( (comp_cmdUnit0, "outBankReqPtr", g_params["clockCycle"]), (comp_dimm0, "inCmdUnitReqPtr", g_params["clockCycle"]) )

# CmdUnit <- Dimm (Res) (Cmd)
cmdResLink_1 = sst.Link("cmdResLink_1")
cmdResLink_1.connect( (comp_cmdUnit0, "inBankResPtr", g_params["clockCycle"]), (comp_dimm0, "outCmdUnitResPtr", g_params["clockCycle"]) )




# End of generated output.
