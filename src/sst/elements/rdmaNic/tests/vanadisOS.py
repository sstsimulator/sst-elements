import os
import sst

debug_addr = 0x6280
stdMem_debug = 0

debugPython=False

physMemSize = "4GiB"
full_exe_name="./app/rdma/msg"
exe_name= full_exe_name.split("/")[-1]

coherence_protocol="MESI"
    
cpu_clock = os.getenv("VANADIS_CPU_CLOCK", "2.3GHz")
os_verbosity = os.getenv("VANADIS_OS_VERBOSE", 0)
               
class Builder:
    def __init__(self):
                pass

    def build( self, nodeId, cpuId ):

        self.prefix = 'node' + str(nodeId)

        self.nodeOS = sst.Component(self.prefix + ".os", "vanadis.VanadisNodeOS")
        self.nodeOS.addParams({
            "verbose" : os_verbosity,
            "cores" : 1,
            "nodeNum" : nodeId,
            "hardwareThreadCount" : 1,
            "heap_start" : 512 * 1024 * 1024,
            "heap_end"   : (2 * 1024 * 1024 * 1024) - 4096,
            "page_size"  : 4096,
            "heap_verbose" : 0, #verbosity
            "process0.env_count" : 2,
            "process0.env0" : "HOME=/home/sdhammo",
            "process0.env1" : "NEWHOME=/home/sdhammo2",
            "process0.exe" : full_exe_name,
            "process0.arg0" : exe_name,
            "physMemSize" : physMemSize,
            "useMMU" : True,
        })
        self.mmu = self.nodeOS.setSubComponent( "mmu", "mmu.simpleMMU" )
        self.mmu.addParams({
            "debug_level": 0,
            "num_cores": 1,
            "num_threads": 1,
            "page_size": 4096,
            "useNicTlb": True,
        })

        mem_if = self.nodeOS.setSubComponent( "mem_interface", "memHierarchy.standardInterface" )
        mem_if.addParam("coreId",cpuId)
        mem_if.addParams({
            "debug" : stdMem_debug,
            "debug_level" : 11,
        })

        l1cache = sst.Component(self.prefix + ".node_os.l1cache", "memHierarchy.Cache")
        l1cache.addParams({
            "access_latency_cycles" : "2",
            "cache_frequency" : cpu_clock,
            "replacement_policy" : "lru",
            "coherence_protocol" : coherence_protocol,
            "associativity" : "8",
            "cache_line_size" : "64",
            "cache_size" : "32 KB",
            "L1" : "1",
            "debug" : 0, 
            "debug_level" : 10,
            "debug_addr": debug_addr
        })

        l1cache_2_cpu = l1cache.setSubComponent("cpulink", "memHierarchy.MemLink")

        link = sst.Link(self.prefix + ".link_os_l1cache")
        link.connect( (mem_if, "port", "1ns"), (l1cache_2_cpu, "port", "1ns") )

        return l1cache  

    def connectCPU( self, core, cpu ):
        if debugPython:
            print( "connectCPU core {}, ".format( core ) )

        link = sst.Link(self.prefix + ".link_core" + str(core) + "_os")
        link.connect( (cpu, "os_link", "5ns"), (self.nodeOS, "core0", "5ns") )

    def connectTlb( self, core, name, tlblink ):
        linkName = self.prefix + ".link_mmu_core" + str(core) + "_" + name

        if debugPython:
            print( "connectTlb {} core {}, name {}, linkName {}".format(self.prefix, core, name, linkName ) )

        link = sst.Link( linkName )
        link.connect( (self.mmu, "core"+str(core)+ "." +name, "1ns"), (tlblink, "mmu", "1ns") )

    def connectNicTlb( self, name, niclink ):
        linkName = self.prefix + ".link_mmu_" + name

        if debugPython:
            print( "connectNicTlb {} , name {}, linkName {}".format(self.prefix, name, linkName ) )

        link = sst.Link( linkName )
        link.connect( (self.mmu, name, "1ns"), (niclink, "mmu", "1ns") )

