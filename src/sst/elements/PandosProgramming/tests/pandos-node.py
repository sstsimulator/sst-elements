# import the SST module
import sst

# create a single pandos node
pn0 = sst.Component("pn0", "PandosProgramming.PandosNodeT")
pn1 = sst.Component("pn1", "PandosProgramming.PandosNodeT")

# parameterize the components
params = {
    "num_cores" : 128,
    "instructions_per_task" : 20,
    "program_binary_fname": "/home/mrutt/work/AGILE/",
}

pn0.addParams(params)
pn1.addParams(params)

# Link the components via their ports
link = sst.Link("component_link")
link.connect( (pn0, "port", "1ns"), (pn1, "port", "1ns") )
