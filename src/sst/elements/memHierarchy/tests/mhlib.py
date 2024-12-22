import sst

### Note: This module is being used to prototype a set of python utilities 
###  for more easily generating and configuring simulations using memHierarchy.
###  Feel free to use the utilities available here but be aware that this file 
###  may change without warning in the SST-Elements repository.
###
### Eventually, a python module with convenience functions for memHierarchy will be released 
###  and that module will be fully supported by the project (provide backwards compatibility, 
###  deprecation notices, stability testing).
###


# List of components in memH, convenient for enabling stats by component
componentlist = (
    "memHierarchy.BroadcastShim",
    "memHierarchy.Bus",
    "memHierarchy.Cache",
    "memHierarchy.CoherentMemController",
    "memHierarchy.DirectoryController",
    "memHierarchy.MemController",
    "memHierarchy.ScratchCPU",
    "memHierarchy.Scratchpad",
    "memHierarchy.Sieve",
    "memHierarchy.multithreadL1",
    "memHierarchy.standardCPU",
    "memHierarchy.streamCPU",
    "memHierarchy.trivialCPU",
    "memHierarchy.DelayBuffer",
    "memHierarchy.IncoherentController",
    "memHierarchy.L1CoherenceController",
    "memHierarchy.L1IncoherentController",
    "memHierarchy.MESICacheDirectoryCoherenceController",
    "memHierarchy.MESICoherenceController",
    "memHierarchy.MemLink",
    "memHierarchy.MemNIC",
    "memHierarchy.MemNICFour",
    "memHierarchy.MemNetBridge",
    "memHierarchy.MemoryManagerSieve",
    "memHierarchy.Messier",
    "memHierarchy.defCustomCmdHandler",
    "memHierarchy.cramsim",
    "memHierarchy.emptyCacheListener",
    "memHierarchy.extMemBackendConvertor",
    "memHierarchy.fifoTransactionQ",
    "memHierarchy.flagMemBackendConvertor",
    "memHierarchy.goblinHMCSim",
    "memHierarchy.hash.linear",
    "memHierarchy.hash.none",
    "memHierarchy.hash.xor",
    "memHierarchy.memInterface",
    "memHierarchy.networkMemoryInspector",
    "memHierarchy.reorderByRow",
    "memHierarchy.reorderSimple",
    "memHierarchy.reorderTransactionQ",
    "memHierarchy.replacement.lfu",
    "memHierarchy.replacement.lru",
    "memHierarchy.replacement.mru",
    "memHierarchy.replacement.nmru",
    "memHierarchy.replacement.rand",
    "memHierarchy.scratchInterface",
    "memHierarchy.simpleDRAM",
    "memHierarchy.simpleMem",
    "memHierarchy.simpleMemBackendConvertor",
    "memHierarchy.simpleMemScratchBackendConvertor",
    "memHierarchy.simplePagePolicy",
    "memHierarchy.standardInterface",
    "memHierarchy.timeoutPagePolicy",
    "memHierarchy.timingDRAM",
    "memHierarchy.vaultsim"
)

class Bus:
    """ MemHierarchy Bus instance with convenience functions for connecting links """
    
    def __init__(self, name, params, latency, highcomps=[], lowcomps=[]):
        """name = name of bus component
           params = parameters for bus component
           latency = default link latency for links to the bus
           highcomps = components to connect on the upper/cpu-side of the bus
           lowcomps = components to connect on the lower/memory-side of the bus
        """
        self.bus = sst.Component(name, "memHierarchy.Bus")
        self.bus.addParams(params)
        self.name = name
        self.highlinks = 0
        self.lowlinks = 0
        self.latency = latency
        
        self.connect(highcomps, lowcomps)
   
    def connect(self, highcomps=[], lowcomps=[], latency=None):
        if latency is None:
            latency = self.latency

        for x in highcomps:
            link = sst.Link("link_" + self.name + "_" + x.getFullName() + "_highlink" + str(self.highlinks)) # port_busname_compname_portnum
            link.connect( (x, "lowlink", latency), (self.bus, "highlink" + str(self.highlinks), latency) )
            self.highlinks = self.highlinks + 1
                                   
        for x in lowcomps:
            link = sst.Link("link_" + self.name + "_" + x.getFullName() + "_lowlink" + str(self.lowlinks)) # port_busname_compname_portnum
            link.connect( (x, "highlink", latency), (self.bus, "lowlink" + str(self.lowlinks), latency) )
            self.lowlinks = self.lowlinks + 1
    




