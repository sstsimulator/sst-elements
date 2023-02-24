# import the SST module
import sst
import sys

# create a single pandos node
pn0 = sst.Component("pn0", "PandosProgramming.PandosNodeT")
#pn1 = sst.Component("pn1", "PandosProgramming.PandosNodeT")

# parameterize the components
params = {
    "num_cores" : 2,
    "instructions_per_task" : 20,
    "program_binary_fname": sys.argv[1],
    "verbose_level" : 4,
    "debug_scheduler" : False,
    "debug_memory_requests" : True,
    "debug_initialization" : True,
}

pn0.addParams(params)
#pn1.addParams(params)

# Link the components via their ports
# link up the core local spm
toCoreLocalSPM = sst.Link("to_core_local_spm_link")
fromCoreLocalSPM = sst.Link("from_core_local_spm_link")
toCoreLocalSPM.connect((pn0, "toCoreLocalSPM", "1ns"), (pn0, "toCoreLocalSPM", "1ns"))
fromCoreLocalSPM.connect((pn0, "fromCoreLocalSPM", "1ns"), (pn0, "fromCoreLocalSPM", "1ns"))
    
# link up the node shared dram
toPodSharedDRAM = sst.Link("to_pod_shared_dram_link")
fromPodSharedDRAM = sst.Link("from_pod_shared_dram_link")
toPodSharedDRAM.connect((pn0, "toNodeSharedDRAM", "35ns"), (pn0, "toNodeSharedDRAM", "35ns"))
fromPodSharedDRAM.connect((pn0, "fromNodeSharedDRAM", "35ns"), (pn0, "fromNodeSharedDRAM", "35ns"))
