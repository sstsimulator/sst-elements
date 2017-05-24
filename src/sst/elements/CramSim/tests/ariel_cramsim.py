import sst 
import os


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
#sst.setProgramOption("stopAtCycle", "21000us")




## Flags
memDebug = 0
memDebugLevel = 10
coherenceProtocol = "MESI"
rplPolicy = "lru"
busLat = "50 ps"
cacheFrequency = "2 Ghz"
defaultLevel = 0
cacheLineSize = 64

corecount = 8

## Application Info
os.environ['SIM_DESC'] = 'EIGHT_CORES'
os.environ['OMP_NUM_THREADS'] = str(corecount)


sst_root = os.getenv( "SST_ROOT" )

## Application Info:
## Executable  -> exe_file
## appargcount -> Number of commandline arguments after <exec_file> name
## apparg<#>   -> arguments 
## Commandline execution for the below example would be 
## /home/amdeshp/arch/benchmarks/PathFinder_1.0.0/PathFinder_ref/PathFinder.x -x /home/amdeshp/arch/benchmarks/PathFinder_1.0.0/generatedData/small1.adj_list
## AppArgs = ({
##    "executable"  : "/home/amdeshp/arch/benchmarks/PathFinder_1.0.0/PathFinder_ref/PathFinder.x",
##    "appargcount" : "0",
##    "apparg0"     : "-x",
##    "apparg1"     : "/home/amdeshp/arch/benchmarks/PathFinder_1.0.0/generatedData/small1.adj_list",
## })

## Processor Model
ariel = sst.Component("A0", "ariel.ariel")
## ariel.addParams(AppArgs)
ariel.addParams({
   "verbose"             : "0",
   "maxcorequeue"        : "256",
   "maxissuepercycle"    : "2",
   "pipetimeout"         : "0",
   "executable"         : "./stream",
   "memorylevels"        : "1",
   "arielinterceptcalls" : "1",
   "arielmode"           : "1",
   "corecount"           : corecount,
   "defaultlevel"        : defaultLevel,
})
 
## MemHierarchy 

def genMemHierarchy(cores):
   
   membus = sst.Component("membus", "memHierarchy.Bus")
   membus.addParams({
       "bus_frequency" : cacheFrequency,
   })
  

   memory = sst.Component("memory", "memHierarchy.MemController")
   memory.addParams({
       "range_start"           : "0",
       "coherence_protocol"    : coherenceProtocol,
       "debug"                 : memDebug,
       "clock"                 : "1Ghz",
      "backend" : "memHierarchy.cramsim",
      "backend.access_time" : "2 ns",   # Phy latency
      "backend.mem_size" : "512MiB",
       "request_width"         : cacheLineSize
   })

   for core in range (cores):
       l1 = sst.Component("l1cache_%d"%core, "memHierarchy.Cache")
       l1.addParams({
           "cache_frequency"       : cacheFrequency,
           "cache_size"            : "32KB",
           "cache_line_size"       : cacheLineSize,
           "associativity"         : "8",
           "access_latency_cycles" : "4",
           "coherence_protocol"    : coherenceProtocol,
           "replacement_policy"    : rplPolicy,
           "L1"                    : "1",
           "debug"                 : memDebug,  
           "debug_level"           : memDebugLevel, 
       })

       l2 = sst.Component("l2cache_%d"%core, "memHierarchy.Cache")
       l2.addParams({
           "cache_frequency"       : cacheFrequency,
           "cache_size"            : "256 KB",
           "cache_line_size"       : cacheLineSize,
           "associativity"         : "8",
           "access_latency_cycles" : "10",
           "coherence_protocol"    : coherenceProtocol,
           "replacement_policy"    : rplPolicy,
           "L1"                    : "0",
           "debug"                 : memDebug,  
           "debug_level"           : memDebugLevel, 
           "mshr_num_entries"      : "16",
       })
       
       ## SST Links
       # Ariel -> L1(PRIVATE) -> L2(PRIVATE)  -> L3 (SHARED) -> DRAM 
       ArielL1Link = sst.Link("cpu_cache_%d"%core)
       ArielL1Link.connect((ariel, "cache_link_%d"%core, busLat), (l1, "high_network_0", busLat))
       L1L2Link = sst.Link("l1_l2_%d"%core)
       L1L2Link.connect((l1, "low_network_0", busLat), (l2, "high_network_0", busLat))
       L2MembusLink = sst.Link("l2_membus_%d"%core)
       L2MembusLink.connect((l2, "low_network_0", busLat), (membus, "high_network_%d"%core, busLat))
   

   l3 = sst.Component("L3cache", "memHierarchy.Cache")
   l3.addParams({
       "cache_frequency"       : cacheFrequency,
       "cache_size"            : "8 MB",
       "cache_line_size"       : cacheLineSize,
       "associativity"         : "8",
       "access_latency_cycles" : "20",
       "coherence_protocol"    : coherenceProtocol,
       "replacement_policy"    : rplPolicy,
       "L1"                    : "0",
       "debug"                 : memDebug,  
       "debug_level"           : memDebugLevel, 
       "mshr_num_entries"      : "16",
   })
  
   # Bus to L3 and L3 <-> MM
   BusL3Link = sst.Link("bus_L3")
   BusL3Link.connect((membus, "low_network_0", busLat), (l3, "high_network_0", busLat))
   L3MemCtrlLink = sst.Link("L3MemCtrl")
   L3MemCtrlLink.connect((l3, "low_network_0", busLat), (memory, "direct_link", busLat))
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

   link_dir_cramsim_link = sst.Link("link_dir_cramsim_link")
   link_dir_cramsim_link.connect( (memory, "cube_link", "2ns"), (comp_memhBridge, "linkCPU", "2ns") )

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





genMemHierarchy(corecount)        

