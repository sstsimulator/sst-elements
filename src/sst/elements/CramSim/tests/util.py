import sys

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

