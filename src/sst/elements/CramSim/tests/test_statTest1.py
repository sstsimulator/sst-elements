#This test file is intended to be run from the verimem python suite within ./Verimem/

# Automatically generated SST Python input
import sst
import sys
import time

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


g_trace_file = ""
g_config_file = ""

# Setup global parameters
[g_config_file, g_overrided_list] = read_arguments()
g_params = setup_config_params(g_config_file, g_overrided_list)


# Define SST core options
sst.setProgramOption("timebase", g_params["clockCycle"])
sst.setProgramOption("stopAtCycle", g_params["stopAtCycle"])

#sst.setStatisticLoadLevel(7)
#sst.setStatisticOutput("sst.statOutputConsole")
#sst.setStatisticOutputOption("help", "help")

# Define the simulation components
# txn gen
comp_txnGen0 = sst.Component("TxnGen", "CramSim.c_TxnGen")
comp_txnGen0.addParams(g_params)
comp_txnGen0.addParams({
        "mode" : "rand",
	"numTxnPerCycle" : 1,
	"readWriteRatio" : 0.5
	})
comp_txnGen0.enableAllStatistics()




# controller
comp_controller0 = sst.Component("MemController0", "CramSim.c_Controller")
comp_controller0.addParams(g_params)
comp_controller0.addParams({
		"TxnScheduler" : "CramSim.c_TxnScheduler",
		"TxnConverter" : "CramSim.c_TxnConverter",
		"AddrMapper" : "CramSim.c_AddressHasher",
		"CmdScheduler" : "CramSim.c_CmdScheduler" ,
		"DeviceDriver" : "CramSim.c_DeviceDriver"
		})



# bank receiver
comp_dimm0 = sst.Component("Dimm0", "CramSim.c_Dimm")
comp_dimm0.addParams(g_params)

'''
# enable all statistics
comp_txnGen0.enableAllStatistics()
comp_controller0.enableAllStatistics()
comp_controller0.enableStatistics(["reqQueueSize"],  # overriding the type of one statistic
                                  {"type":"sst.HistogramStatistic",
                                   "minvalue" : "0",
                                   "binwidth" : "2",
                                   "numbins" : "18",
                                   "IncludeOutOfBounds" : "1"})
comp_controller0.enableStatistics(["resQueueSize"],  # overriding the type of one statistic
                                  {"type":"sst.HistogramStatistic",
                                   "minvalue" : "0",
                                   "binwidth" : "2",
                                   "numbins" : "18",
                                   "IncludeOutOfBounds" : "1"})
#comp_txnUnit0.enableAllStatistics({ "type":"sst.AccumulatorStatistic",
#                                    "rate":"1 us"})
comp_dimm0.enableAllStatistics()
'''


# Define simulation links

# TXNGEN / Controller LINKS
# TxnGen <-> Controller
txnReqLink_0 = sst.Link("txnReqLink_0")
txnReqLink_0.connect( (comp_txnGen0, "memLink", g_params["clockCycle"]), (comp_controller0, "txngenLink", g_params["clockCycle"]) )


# Controller <-> Dimm
cmdReqLink_1 = sst.Link("cmdReqLink_1")
cmdReqLink_1.connect( (comp_controller0, "memLink", g_params["clockCycle"]), (comp_dimm0, "ctrlLink", g_params["clockCycle"]) )





# End of generated output.
