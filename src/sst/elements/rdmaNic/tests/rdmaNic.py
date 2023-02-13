import os
import sst

cpu_clock = os.getenv("VANADIS_CPU_CLOCK", "2.3GHz")
coherence_protocol="MESI"

stdMem_debug = 0
nicCache_debug = 0
debug_level = 11

debug_addr=0x6280

debugPython=False

tlbParams = {
    "debug_level": 0,
    "hitLatency": 10,
    "num_hardware_threads": 1,
    "num_tlb_entries_per_thread": 64,
    "tlb_set_size": 4,
}

tlbWrapperParams = {
    "debug_level": 0,
}


class Builder:
    def __init__(self,numNodes):
        self.numNodes = numNodes

    def build( self, nodeId ):

        if debugPython:
            print("nodeId {}".format(nodeId ))

        prefix = 'node' + str(nodeId)
        nic = sst.Component( prefix + ".nic", "rdmaNic.nic")
        nic.addParams({
            "clock" : "1GHz",
            "debug_level": 0,
            "useDmaCache": "true",
            "debug_mask": -1,
            "maxPendingCmds" : 128,
            "maxMemReqs" : 256,
            "maxCmdQSize" : 128,
            "cache_line_size"    : 64,
            # "addr_range_start" : 0,
            # "addr_range_end" : 0x7fffffff,
            'baseAddr': 0x80000000,
            'cmdQSize' : 64,
	})
        nic.addParam( 'nicId', nodeId )
        nic.addParam( 'pesPerNode', 1 )
        nic.addParam( 'numNodes', self.numNodes )


        # NIC DMA interface
        dmaIf = nic.setSubComponent("dma", "memHierarchy.standardInterface")
        dmaIf.addParams({
            "debug" : stdMem_debug,
            "debug_level" : debug_level,
        })

        # NIC MMIO interface
        mmioIf = nic.setSubComponent("mmio", "memHierarchy.standardInterface")
        mmioIf.addParams({
            #"debug" : stdMem_debug,
            "debug" : 0,
            "debug_level" : 10,
        })

        # NIC DMA Cache
        dmaCache = sst.Component(prefix + ".nicDmaCache", "memHierarchy.Cache")
        dmaCache.addParams({
            "access_latency_cycles" : "1",
            "access_latency_cycles" : "2",
            "cache_frequency" : cpu_clock,
            "replacement_policy" : "lru",
            "coherence_protocol" : coherence_protocol,
            "associativity" : "8",
            #"associativity" : "16",
            "cache_line_size" : "64",
            #"cache_size" : "8MB",
            "cache_size" : "32KB",
            "L1" : "1",
            "debug": nicCache_debug,
            "debug_level" : debug_level,
            "debug_addr" : debug_addr,
        })

        # NIC DMA TLB
        tlbWrapper = sst.Component(prefix+".nicDmaTlb", "mmu.tlb_wrapper")
        tlbWrapper.addParams(tlbWrapperParams)
        tlb = tlbWrapper.setSubComponent("tlb", "mmu.simpleTLB" );
        tlb.addParams(tlbParams)

        # Cache to CPU interface
        dmaCacheToCpu = dmaCache.setSubComponent("cpulink", "memHierarchy.MemLink")

        # NIC DMA -> TLB
        link = sst.Link(prefix+".link_cpu_dtlb")
        link.connect( (dmaIf, "port", "1ns"), (tlbWrapper, "cpu_if", "1ns") )

        # NIC DMA TLB -> cache
        link = sst.Link(prefix+".link_cpu_l1dcache")
        link.connect( (tlbWrapper, "cache_if", "1ns"), (dmaCacheToCpu, "port", "1ns") )
	
        # NIC internode interface 
        netLink = nic.setSubComponent( "rtrLink", "merlin.linkcontrol" )
        netLink.addParam("link_bw","16GB/s")
        netLink.addParam("input_buf_size","14KB")
        netLink.addParam("output_buf_size","14KB")

        return mmioIf, dmaCache, tlb, (netLink, "rtr_port", '10ns')
