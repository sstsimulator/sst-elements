import sys

def read_arguments():
	txnGen = ""
	trace_file = ""
	config_file = ""
	stopAtCycle = ""
	boolDefaultConfig = True;
	boolDefaultTxnGen = True;
	boolDefaultTrace = True;
	txngen_option = "seq";

	for arg in sys.argv:
		if arg.find("--configfile=") != -1:
			substrIndex = arg.find("=")+1
			config_file = arg[substrIndex:]
			print "Config file:", config_file
			boolDefaultConfig = False;
		
		if arg.find("--txngen=") != -1:
			substrIndex = arg.find("=")+1
			txngen_option = arg[substrIndex:]
			boolDefaultTxnGen = False;
			if txngen_option == "seq" :
				txnGen = "CramSim.c_TxnGenSeq"
			elif txngen_option == "rand" :
				txnGen = "CramSim.c_TxnGenRand"
			elif txngen_option == "trace" :
				txnGen = "CramSim.c_DramSimTraceReader"

		if arg.find("--tracefile=") != -1:
			substrIndex = arg.find("=")+1
			trace_file = arg[substrIndex:]
			boolDefaultTrace = False;
			print "Trace file:", trace_file

		if arg.find("--stopAtCycle=") != -1:
			substrIndex = arg.find("=")+1
			stopAtCycle = arg[substrIndex:]
			print "stopAtCycle", stopAtCycle


	if boolDefaultTxnGen == True:
		txnGen = "CramSim.c_TxnGenRand"
		print "Txn Generator is not specified.. using random address generator"
	
	if boolDefaultConfig == True:
		config_file = "../ddr4_verimem.cfg"
		print "config file is not specified.. using ddr4_verimem.cfg"

	if boolDefaultTrace == True :
		if txngen_option == "trace":
			print "Trace file is not specified... error!!"
			sys.exit()

	return [trace_file, config_file, txnGen, stopAtCycle]



def setup_config_params(config_file):
    l_params = {}
    l_configFile = open(config_file, 'r')
    for l_line in l_configFile:
        l_tokens = l_line.split()
         #print l_tokens[0], ": ", l_tokens[1]
        l_params[l_tokens[0]] = l_tokens[1]

    return l_params

