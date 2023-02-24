# import the SST module
import sst
import sys

class PXN(object):
    def __init__(self
                 ,component_name
                 ,params
    ):
        self.component = sst.Component(component_name, "PandosProgramming.PandosNodeT")
        self.component_name = component_name
        self.params = params
        self.component.addParams(self.params)
        # Link the components via their ports
        # link up the core local spm
        self.toCoreLocalSPM = sst.Link(self.component_name + "_to_core_local_spm_link")
        self.fromCoreLocalSPM = sst.Link(self.component_name + "_from_core_local_spm_link")
        self.toCoreLocalSPM.connect((self.component, "toCoreLocalSPM", "1ns"), (self.component, "toCoreLocalSPM", "1ns"))
        self.fromCoreLocalSPM.connect((self.component, "fromCoreLocalSPM", "1ns"), (self.component, "fromCoreLocalSPM", "1ns"))
        # link up the node shared dram
        self.toPodSharedDRAM = sst.Link(self.component_name + "_to_pod_shared_dram_link")
        self.fromPodSharedDRAM = sst.Link(self.component_name + "_from_pod_shared_dram_link")
        self.toPodSharedDRAM.connect((self.component, "toNodeSharedDRAM", "35ns"), (self.component, "toNodeSharedDRAM", "35ns"))
        self.fromPodSharedDRAM.connect((self.component,"fromNodeSharedDRAM", "35ns"), (self.component, "fromNodeSharedDRAM", "35ns"))

def PXN_connect(pxn0, pxn1):
    _0_to_1 = sst.Link(pxn0.component_name + "__to__" + pxn1.component_name)
    _1_to_0 = sst.Link(pxn1.component_name + "__to__" + pxn0.component_name)
    _0_to_1.connect(
        (pxn0.component, "requestsToRemoteNode", "100ns"),
        (pxn1.component, "requestsFromRemoteNode", "100ns")
    )
    _1_to_0.connect(
        (pxn1.component, "requestsToRemoteNode", "100ns"),
        (pxn0.component, "requestsFromRemoteNode", "100ns")
    )
    pxn0.toRemoteNode = _0_to_1
    pxn0.fromRemoteNode = _1_to_0
    pxn1.toRemoteNode = _1_to_0
    pxn1.fromRemoteNode = _0_to_1
        
# parameterize the components
params = {
    "num_cores" : 2,
    "instructions_per_task" : 20,
    "program_binary_fname": sys.argv[1],
    "verbose_level" : 2,
    "debug_scheduler" : False,
    "debug_memory_requests" : True,
    "debug_initialization" : True,
}

pn0 = PXN("pn0", params)
pn1 = PXN("pn1", params)
PXN_connect(pn0, pn1)
