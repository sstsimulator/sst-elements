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
sst.setProgramOption("stopAtCycle", "110000ns")



# Define the simulation components
comp_cpu = sst.Component("cpu", "memHierarchy.trivialCPU")
comp_cpu.addParams({
      "do_write" : "1",
      "num_loadstore" : "1000",
      "commFreq" : "100",
      "memSize" : "0x1000"
})
comp_l1cache = sst.Component("l1cache", "memHierarchy.Cache")
comp_l1cache.addParams({
      "access_latency_cycles" : "4",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "32",
      "debug" : "0",
      "L1" : "1",
      "cache_size" : "2 KB"
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


# Define the simulation links
link_cpu_cache_link = sst.Link("link_cpu_cache_link")
link_cpu_cache_link.connect( (comp_cpu, "mem_link", "1000ps"), (comp_l1cache, "high_network_0", "1000ps") )
link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (comp_l1cache, "low_network_0", "50ps"), (comp_memory, "direct_link", "50ps") )

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
