# import the SST module
import sst
import sys

# create a single pandos node
pn0 = sst.Component("pn0", "PandosProgramming.PandosNodeT")
#pn1 = sst.Component("pn1", "PandosProgramming.PandosNodeT")

# parameterize the components
params = {
    "num_cores" : 1,
    "instructions_per_task" : 20,
    "program_binary_fname": sys.argv[1],
}

pn0.addParams(params)
#pn1.addParams(params)

# Link the components via their ports
coreLocalSPM = sst.Link("component_link")
coreLocalSPM.connect((pn0, "coreLocalSPM", "2ns"), (pn0, "coreLocalSPM", "2ns"))

