import os
import sst
import sys

debug_addr = 0x6280
stdMem_debug = 0

debugPython=False

physMemSize = "4GiB"
full_exe_name= os.getenv("RDMANIC_EXE", "./app/rdma/riscv64/msg" )
exe_name= full_exe_name.split("/")[-1]

coherence_protocol="MESI"

cpu_clock = os.getenv("VANADIS_CPU_CLOCK", "2.3GHz")
os_verbosity = os.getenv("VANADIS_OS_VERBOSE", 0)

app_args = (exe_name + " " + os.getenv("VANADIS_EXE_ARGS", "" )).split()

# MUSL libc uses this in localtime, if we don't set TZ=UTC we
# can get different results on different systems
app_env = ("TZ=UTC " + os.getenv("VANADIS_EXE_ENV", "" )).split()

class Builder:
    def __init__(self):
                pass

    def build( self, numNodes, nodeId, cpuId ):

        self.prefix = 'node' + str(nodeId)

        self.nodeOS = sst.Component(self.prefix + ".os", "vanadis.VanadisNodeOS")
        self.nodeOS.addParams({
            "node_id": nodeId,
            "dbgLevel" : os_verbosity,
            "dbgMask" : -1,
            "cores" : 1,
            "hardwareThreadCount" : 1,
            "page_size"  : 4096,
            "physMemSize" : physMemSize,
            "process0.exe" : full_exe_name,
            "useMMU" : True,
        })

        cnt = 0
        for value in app_args:
            key= "process0.arg" + str(cnt);
            self.nodeOS.addParam( key, value )
            cnt += 1

        self.nodeOS.addParam( "process0.argc", cnt )

        cnt = 0
        for value in app_env:
            key= "process0.env" + str(cnt);
            self.nodeOS.addParam( key, value )
            cnt += 1


        # for mvapich runtime
        self.nodeOS.addParam(  "process0.env" + str(cnt), "PMI_SIZE=" + str(numNodes) )
        cnt += 1

        self.nodeOS.addParam(  "process0.env" + str(cnt), "PMI_RANK=" + str(nodeId) )
        cnt += 1

        self.nodeOS.addParam( "process0.env_count", cnt )

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

        link = sst.Link(self.prefix + ".link_os_l1cache")
        link.connect( (mem_if, "lowlink", "1ns"), (l1cache, "highlink", "1ns") )

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

