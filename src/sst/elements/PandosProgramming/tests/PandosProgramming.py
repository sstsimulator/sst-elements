import sst
import sys


class Node(object):
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

    def connect(self, other):
        self_to_other = sst.Link(self.component_name + "__to__" + other.component_name)
        other_to_self = sst.Link(other.component_name + "__to__" + self.component_name)
        self_to_other.connect(
            (self.component, "requestsToRemoteNode", "100ns"),
            (self.component, "requestsFromRemoteNode", "100ns")
        )
        other_to_self.connect(
            (other.component, "requestsToRemoteNode", "100ns"),
            (other.component, "requestsFromRemoteNode", "100ns")
        )
        self.toRemoteNode = self_to_other
        self.fromRemoteNode = other_to_self
        other.toRemoteNode = other_to_self
        other.fromRemoteNode = self_to_other

def program_fpath():
    return sys.argv[1]

def program_argv():
    return " ".join(sys.argv[2:]) if len(sys.argv) > 1 else ""


class NodeFactory(object):
    def __init__(self, params):
        self.params =  params
        self.nodes = []

    def createNode(self, node_name):
        params = self.params.copy()
        params['node_id']=len(self.nodes)
        new_node = Node(node_name, params)
        self.nodes.append(new_node)
        return new_node
    
    def done(self):
        for node in self.nodes:
            node.component.addParams({'sys_num_nodes':len(self.nodes)})
