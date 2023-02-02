import os
import sst

coherence_protocol="MESI"

dc_debug = 0 
mc_debug = 0 
stdMem_debug = 0 

debug_addr = 0x6280

debugPython=False

cpu_clock = os.getenv("VANADIS_CPU_CLOCK", "2.3GHz")

class Builder:
    def __init__(self):
        pass

    def build( self, nodeId, numPorts,  group  ):

        self.prefix = 'node' + str(nodeId) 
        self.numPorts = numPorts + 1
        if debugPython:
            print("Memory nodeid={} numPorts={}".format(nodeId,numPorts))

        self.chiprtr = sst.Component(self.prefix + ".chiprtr", "merlin.hr_router")
        self.chiprtr.addParams({
            "xbar_bw" : "50GB/s",
            "link_bw" : "25GB/s",
            "input_buf_size" : "40KB",
            "output_buf_size" : "40KB",
            "num_ports" : str(self.numPorts + 1),
            "flit_size" : "72B",
            "id" : "0",
            "topology" : "merlin.singlerouter"
        })

        self.chiprtr.setSubComponent("topology","merlin.singlerouter")

        dirctrl = sst.Component(self.prefix + ".dirctrl", "memHierarchy.DirectoryController")
        dirctrl.addParams({
            "coherence_protocol" : coherence_protocol,
            "entry_cache_size" : "1024",
            "addr_range_start" : "0x0",
            "addr_range_end" : "0x7fffffff",
            "debug": dc_debug,
            "debug_level" : 11,
            "debug_addr" : debug_addr,
        })
        dirtoMemLink = dirctrl.setSubComponent("memlink", "memHierarchy.MemLink")
        self.connect( "Dirctrl", numPorts, dirctrl, group, linkType="cpulink" ) 

        memctrl = sst.Component(self.prefix + ".memory", "memHierarchy.MemController")
        memctrl.addParams({
            "clock" : cpu_clock,
            "backend.mem_size" : "4GiB",
            "backing" : "malloc",
            "addr_range_start" : "0x0",
            "addr_range_end" : "0x7fffffff",
            "debug" : mc_debug,
            "debug_level" : 11,
            "debug_addr" : debug_addr,
            "initBacking": 1,
        })

        memToDir = memctrl.setSubComponent("cpulink", "memHierarchy.MemLink")

        memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
        memory.addParams({
            "mem_size" : "2GiB",
            "access_time" : "1 ns",
            "debug" : 0,
            "debug_level" : 10,
        })

        link = sst.Link(self.prefix + ".link_dir_mem")
        link.connect( (dirtoMemLink, "port", "1ns"), (memToDir, "port", "1ns") )

    def connect( self, name, port, comp, group=None, source=None, dest=None, linkType="memlink"  ):

        assert group
        assert port < self.numPorts
        if debugPython:
            print( "Memory {} connect {} to port {}".format( self.prefix, name, port ) )

        config="{} MemNIC config: groupt={} ".format(name,group)

        memNIC = comp.setSubComponent(linkType, "memHierarchy.MemNIC")
        memNIC.addParams({
            "group" : group,
            "network_bw" : "25GB/s",
            "debug": 0,
            "debug_level": 10,
        })
        if source:
            config+=", sources={}".format(source)
            memNIC.addParam( "sources" , source)
        if dest:
            config+=", destinations={}".format(dest)
            memNIC.addParam( "destinations" , source)
        

        if debugPython:
            print( config )

        link = sst.Link(self.prefix + ".link_rtr" + str(port) )
        link.connect( (self.chiprtr, "port" + str(port), "1ns"), (memNIC, "port", "1ns") )
