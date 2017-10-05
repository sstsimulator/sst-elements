import sst
import sys
import time
################################################################################
def read_arguments():
	config_file_list = list()
        override_list = list()
        boolDefaultConfig = True;

	for arg in sys.argv:
                if arg.find("--configfile=") != -1:
                        substrIndex = arg.find("=")+1
                        config_file_list.append(arg[substrIndex:])
                        print "Config file list:", config_file_list
                        boolDefaultConfig = False;

                elif arg.find("--traceFile=") != -1:  # just remove the -- argument signifier from the beginning
                        substrIndex = arg.find("-")+2  
                        override_list.append(arg[substrIndex:])
                        print "Trace file:", arg[substrIndex:]
                        
                elif arg != sys.argv[0]:
                        if arg.find("=") == -1:
                                print "Malformed config override found!: ", arg
                                exit(-1)
                        override_list.append(arg)
                        print "Override: ", override_list[-1]


	
	if boolDefaultConfig == True:
		config_file_list = "../ddr4_verimem.cfg"
		print "config file is not specified.. using ddr4_verimem.cfg"

	return [config_file_list, override_list]



def setup_config_params(config_file_list, override_list):
    l_params = {}
    for l_configFileEntry in config_file_list:
            l_configFile = open(l_configFileEntry, 'r')
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
g_config_file_list = ""
g_override_list = ""

# Setup global parameters
[g_config_file_list, g_overrided_list] = read_arguments()
g_params = setup_config_params(g_config_file_list, g_overrided_list)
if "dumpConfig" in g_params and int(g_params["dumpConfig"]):
        print "\n###########################\nDumping global config parameters:"
        for key in g_params:
                print key + " " + g_params[key]
        print "###########################\n"

numChannels = int(g_params["numChannels"])
maxOutstandingReqs = numChannels*128
numTxnPerCycle = numChannels
maxTxns = 100000 * numChannels


# Define SST core options
sst.setProgramOption("timebase", g_params["clockCycle"])
sst.setProgramOption("stopAtCycle", g_params["stopAtCycle"])
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")


#########################################################################################################

## Configure transaction generator
comp_txnGen = sst.Component("TxnGen", "CramSim.c_TraceFileReader")
comp_txnGen.addParams(g_params)
comp_txnGen.addParams({
	"maxTxns" : maxTxns,
	"numTxnPerCycle" : numTxnPerCycle,
	"maxOutstandingReqs" : maxOutstandingReqs
	})
comp_txnGen.enableAllStatistics()


# controller
comp_controller = sst.Component("MemController"+"0", "CramSim.c_Controller")
comp_controller.addParams(g_params)
comp_controller.addParams({
		"TxnScheduler" : "CramSim.c_TxnScheduler",
		"TxnConverter" : "CramSim.c_TxnConverter",
		"AddrMapper" : "CramSim.c_AddressHasher",
		"CmdScheduler" : "CramSim.c_CmdScheduler" ,
		"DeviceDriver" : "CramSim.c_DeviceDriver"
		})

# device
comp_dimm = sst.Component("Dimm"+"0", "CramSim.c_Dimm")
comp_dimm.addParams(g_params)

# TXNGEN / Controller LINKS
# TxnGen <-> Controller (Txn)
txnReqLink_0 = sst.Link("txnReqLink_0_"+"0")
txnReqLink_0.connect((comp_txnGen, "memLink", g_params["clockCycle"]), (comp_controller, "txngenLink", g_params["clockCycle"]) )


# Controller <-> Dimm 
cmdReqLink_1 = sst.Link("cmdReqLink_1_"+"0")
cmdReqLink_1.connect( (comp_controller, "memLink", g_params["clockCycle"]), (comp_dimm, "ctrlLink", g_params["clockCycle"]) )


# enable all statistics
comp_controller.enableAllStatistics()
#comp_txnUnit0.enableAllStatistics({ "type":"sst.AccumulatorStatistic",
#                                    "rate":"1 us"})
#comp_dimm.enableAllStatistics()
