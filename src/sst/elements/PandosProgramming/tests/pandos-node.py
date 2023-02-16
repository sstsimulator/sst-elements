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
# link up the core local spm
coreLocalSPM = sst.Link("core_local_spm_link")
coreLocalSPM.connect((pn0, "coreLocalSPM", "2ns"), (pn0, "coreLocalSPM", "2ns"))
# link up the node shared dram
podSharedDRAM = sst.Link("pod_shared_dram_link")
podSharedDRAM.connect((pn0, "podSharedDRAM", "35ns"), (pn0, "podSharedDRAM", "35ns"))
