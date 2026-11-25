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
    """ MemHierarchy Bus instance with convenience functions for connecting links
        Bus may be created

            Ex. (no link params)
            bus_params = {"bus_frequency" : "3GHz", "debug" : DEBUG_BUS, "debug_level" : 10}
            l2_bus = Bus("l2bus", bus_params, "100ps", [l2_cache0, l2_cache1, l2_cache2, l2_cache3])

            Ex. (link params via tuples)
            link_params = {"debug" : DEBUG_LINK, "debug_level" : 10}
            bus_params = {"bus_frequency" : "3GHz", "debug" : DEBUG_BUS, "debug_level" : 10}
            l2_bus = Bus("l2bus", bus_params, "100ps", [(l2_cache0,link_params), (l2_cache1,link_params), (l2_cache2,link_params), (l2_cache3,link_params)])

            Ex. (link params via argument)
            link_params = {"debug" : DEBUG_LINK, "debug_level" : 10}
            bus_params = {"bus_frequency" : "3GHz", "debug" : DEBUG_BUS, "debug_level" : 10}
            l2_bus = Bus("l2bus", bus_params, "100ps", [l2_cache0, l2_cache1, l2_cache2, l2_cache3], link_params=link_params)

            Ex. (add another component to bus after creation)
            l2_bus = Bus("l2bus", bus_params, "100ps", [l2_cache0, l2_cache1, l2_cache2, l2_cache3], [l3cache_0, l3cache_1])
            l2_bus.connect([], [l3cache_2, l3cache_3])
    """

    def __init__(self, name, params, latency, highcomps=[], lowcomps=[], link_params={}):
        """name = name of bus component
           params = parameters for bus component
           latency = default link latency for links to the bus
           highcomps = components to connect on the upper/cpu-side of the bus
                       to add parameters to link, make this an array of (component,params) tuples
           lowcomps = components to connect on the lower/memory-side of the bus
                       to add parameters to link, make this an array of (component,params) tuples
           link_params = a set of parameters to give every link object (e.g., {"debug" : 1, "debug_level" : 10})
        """
        self.bus = sst.Component(name, "memHierarchy.Bus")
        self.bus.addParams(params)
        self.name = name
        self.highlinks = 0
        self.lowlinks = 0
        self.latency = latency
        self.global_link_params = link_params

        self.connect(highcomps, lowcomps)

        """
           highcomps = components to connect on the upper/cpu-side of the bus
                       to add parameters to link, make this an array of (component,params) tuples
           lowcomps = components to connect on the lower/memory-side of the bus
                       to add parameters to link, make this an array of (component,params) tuples
           latency = link latency to use; if None, the Bus's latency will be used
           link_params = a set of parameters to give every link object (e.g., {"debug" : 1, "debug_level" : 10})
                       These will be appended to any link_params passd to the Bus in its constructor
        """
    def connect(self, highcomps=[], lowcomps=[], latency=None, link_params={}):
        if latency is None:
            latency = self.latency

        for x in highcomps:
            params = self.global_link_params
            params.update(link_params)

            if isinstance(x, tuple):
                params.update(x[1])
                comp = x[0]
            else:
                comp = x

            use_subcomp = bool(params)
            linkname = ("link_" + self.name + "_" + comp.getFullName() + "_highlink" + str(self.highlinks)).replace(":", ".")
            linkname = linkname.replace("[","").replace("]","")
            link = sst.Link(linkname)
            if bool(params):
                subcomp = comp.setSubComponent("lowlink", "memHierarchy.MemLink")
                subcomp.addParams(params)
                link.connect( (subcomp, "port", latency), (self.bus, "highlink" + str(self.highlinks), latency) )
            else:
                link.connect( (comp, "lowlink", latency), (self.bus, "highlink" + str(self.highlinks), latency) )

            self.highlinks = self.highlinks + 1

        for x in lowcomps:
            params = self.global_link_params
            params.update(link_params)

            if isinstance(x, tuple):
                params.update(x[1])
                comp = x[0]
            else:
                comp = x

            linkname = ("link_" + self.name + "_" + comp.getFullName() + "_lowlink" + str(self.lowlinks)).replace(":", ".")
            linkname = linkname.replace("[","").replace("]","")
            link = sst.Link(linkname)
            if bool(params):
                subcomp = comp.setSubComponent("highlink", "memHierarchy.MemLink")
                subcomp.addParams(params)
                link.connect( (subcomp, "port", latency), (self.bus, "lowlink" + str(self.lowlinks), latency) )
            else:
                link.connect( (comp, "highlink", latency), (self.bus, "lowlink" + str(self.lowlinks), latency) )

            self.lowlinks = self.lowlinks + 1





