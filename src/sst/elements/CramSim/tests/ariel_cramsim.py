import sst
import sys
import time
import os

#######################################################################################################
def read_arguments():
	config_file = list()
        override_list = list()
        boolDefaultConfig = True;

	for arg in sys.argv:
            if arg.find("--configfile=") != -1:
		substrIndex = arg.find("=")+1
		config_file = arg[substrIndex:]
		print "Config file:", config_file
		boolDefaultConfig = False;

  	    elif arg != sys.argv[0]:
                if arg.find("=") == -1:
                    print "Malformed config override found!: ", arg
                    exit(-1)
                override_list.append(arg)
                print "Override: ", override_list[-1]

	
	if boolDefaultConfig == True:
		config_file = "../ddr4_verimem.cfg"
		print "config file is not specified.. using ddr4_verimem.cfg"

	return [config_file, override_list]



def setup_config_params(config_file, override_list):
    l_params = {}
    l_configFile = open(config_file, 'r')
    for l_line in l_configFile:
        l_tokens = l_line.split()
         #print l_tokens[0], ": ", l_tokens[1]
        l_params[l_tokens[0]] = l_tokens[1]

    for override in override_list:
        l_tokens = override.split("=")
        print "Override cfg", l_tokens[0], l_tokens[1]
        l_params[l_tokens[0]] = l_tokens[1]
     
    return l_params

#######################################################################################################



# Command line arguments
g_boolUseDefaultConfig = False
g_config_file = "../ddr4.cfg"
g_override_list = ""

# Setup global parameters
#[g_boolUseDefaultConfig, g_config_file] = read_arguments()
[g_config_file, g_overrided_list] = read_arguments()
g_params = setup_config_params(g_config_file, g_overrided_list)

# Define SST core options
sst.setProgramOption("timebase", "1ps")
#sst.setProgramOption("stopAtCycle", "21000us")




## Flags
memDebug = 1
memDebugLevel = 1
coherenceProtocol = "MESI"
rplPolicy = "lru"
busLat = "50 ps"
cacheFrequency = "2 Ghz"
defaultLevel = 0
cacheLineSize = 64

corecount = 1

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
   "verbose"             : "1",
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
      "backend.max_outstanding_requests" : 256,
	"backend.verbose" : 1,
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

    # txn gen --> memHierarchy Bridge
   comp_memhBridge = sst.Component("memh_bridge", "CramSim.c_MemhBridge")
   comp_memhBridge.addParams(g_params);
   comp_memhBridge.addParams({
                        "verbose" : "0",
                        "numTxnPerCycle" : g_params["numChannels"],
                        "strTxnTraceFile" : "arielTrace",
                        "boolPrintTxnTrace" : "1"
                        })
   # controller
   comp_controller0 = sst.Component("MemController0", "CramSim.c_Controller")
   comp_controller0.addParams(g_params)
   comp_controller0.addParams({
                        "verbose" : "0",
			"TxnConverter" : "CramSim.c_TxnConverter",
			"AddrMapper" : "CramSim.c_AddressHasher",
			"CmdScheduler" : "CramSim.c_CmdScheduler" ,
			"DeviceController" : "CramSim.c_DeviceController"
			})


		# bank receiver
   comp_dimm0 = sst.Component("Dimm0", "CramSim.c_Dimm")
   comp_dimm0.addParams(g_params)


   link_dir_cramsim_link = sst.Link("link_dir_cramsim_link")
   link_dir_cramsim_link.connect( (memory, "cube_link", "2ns"), (comp_memhBridge, "cpuLink", "2ns") )

   # memhBridge(=TxnGen) <-> Memory Controller 
   memHLink = sst.Link("memHLink_1")
   memHLink.connect( (comp_memhBridge, "memLink", g_params["clockCycle"]), (comp_controller0, "txngenLink", g_params["clockCycle"]) )

   # Controller <-> Dimm
   cmdLink = sst.Link("cmdLink_1")
   cmdLink.connect( (comp_controller0, "memLink", g_params["clockCycle"]), (comp_dimm0, "ctrlLink", g_params["clockCycle"]) )


   comp_controller0.enableAllStatistics()
   comp_memhBridge.enableAllStatistics()
   comp_dimm0.enableAllStatistics()


sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")

genMemHierarchy(corecount)        

