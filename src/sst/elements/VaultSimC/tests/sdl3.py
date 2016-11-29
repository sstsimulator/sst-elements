import sst
import math
from optparse import OptionParser

op = OptionParser()
op.add_option("-a", "--algorithm", action="store", type="int", dest="algo", default=1)
op.add_option("-t", "--num_thr", action="store", type="int", dest="thr", default=16)
op.add_option("-r", "--remap", action="store", type="int", dest="re", default=0)
op.add_option("-p", "--pims", action="store", type="int", dest="pims", default=3)
(options, args) = op.parse_args()

# globals
CPUQuads = 1  # "Quads" per CPU
PIMQuads = 0  # "Quads per PIM
PIMs = options.pims      # number of PIMs
ccLat = "5ns" # cube to cube latency i.e. PIM to PIM

vaultsPerCube = 8 
fakeCPU_DC = 0
fakePIM_DC = 0
coreNetBW = "36GB/s"
memNetBW = "36GB/s"
xbarBW = coreNetBW
flit_size = "72B"
coherence_protocol = "MESI"
busLat = "50ps"
netLat = "6ns"
useAriel = 0
useVaultSim = 1
baseclock = 1500  # in MHz
clock = "%g MHz"%(baseclock)
l2clock = "%g MHz"%(baseclock *0.25)
memclock = "500 MHz"
memDebug = 0
memDebugLevel = 6

rank_size = 512 / PIMs
interleave_size = 1024*4
corecount = (CPUQuads + (PIMQuads * PIMs)) * 4

class corePorts:
    def __init__(self):
        self._next = 0 
    def nextPort(self):
        res = self._next
        self._next = self._next + 1
        return res
coreCtr = corePorts()

# common
memParams = {
        "access_time" : "50ns"
        }
l1PrefetchParams = { }
l2PrefetchParams = {
    #"prefetcher": "cassini.StridePrefetcher",
    #"reach": 8
    }
cpuParams = {    
    "verbose" : "0",
    "workPerCycle" : "1000",
    "commFreq" : "100",
    "memSize" : "0x1000000",
    "do_write" : "1",
    "num_loadstore" : "100000"
    }
#figure number of local ports
if CPUQuads == PIMQuads + 1:
    localPorts = CPUQuads
elif CPUQuads > PIMQuads + 1:
    localPorts = CPUQuads
    fakePIM_DC = localPorts - PIMQuads - 1
else:
    localPorts = PIMQuads + 1
    fakeCPU_DC = localPorts - CPUQuads

routerParams = {"topology": "merlin.torus",
                "link_bw": coreNetBW,
                "xbar_bw": xbarBW,
                "input_latency": "6ns",
                "output_latency": "6ns",
                "input_buf_size" : "4KiB",
                "output_buf_size" : "4KiB",
                "flit_size" : flit_size,
                "torus:shape" : PIMs+1,
                "torus:width" : 1,
                "torus:local_ports": localPorts ,
                "num_ports" : localPorts + 2}

def makeAriel():
    ariel = sst.Component("a0", "ariel.ariel")
    ariel.addParams({
            "verbose" : 5,
            "clock" : clock,
            "maxcorequeue" : 256,
            "maxissuepercycle" : 6,
            "pipetimeout" : 10,
            "fullproc" : (CPUQuads+PIMQuads) * 4,
            #"executable" : "/home/student/tlvlstream/ministream",
            "corecount" : corecount,
            "arielmode" : 0,
            #"arieltool": "/home/student/sst-simulator/tools/ariel/fesimple/fesimple.so"
            })
    print("fullproc " + str((CPUQuads+PIMQuads)*4) + "\n\n")
    if options.re == 1:
        print("Remap\n")
        ariel.addParams({"arieltool": "/home/afrodri/sst-simulator/tools/ariel/fesimple/fesimple_r.so"})
    else:
        print("No Remap\n")
        #ariel.addParams({"arieltool": "/home/afrodri/sst-simulator/tools/ariel/fesimple/fesimple.so"})
        ariel.addParams({"arieltool": "/Users/afrodri/Public/sst/sst-simulator/tools/ariel/fesimple/fesimple.dylib"})

    thr = str(options.thr)
    if options.algo == 1:
        print("GUPS\n")
        ariel.addParams({"executable" : "/Users/afrodri/bench2/sstgups." + thr})
    elif options.algo == 2:
        print("stream\n")
        ariel.addParams({"executable" : "/home/afrodri/bench2/ministream." + thr})
    elif options.algo == 3:
        print("PathFinder\n")
        ariel.addParams({"executable" : "/home/afrodri/bench2/PathFinder." + thr + ".x"})
        ariel.addParams({"appargcount" : 2,
                         "apparg0" : "-x",
                         "apparg1" : "medsmall1.adj_list"})
    elif options.algo == 4:
        print("miniFE\n")
        ariel.addParams({"executable" : "/home/afrodri/bench2/miniFE." + thr + ".x"})
    elif options.algo == 5:
        print("lulesh\n")
        ariel.addParams({"executable" : "/home/afrodri/bench2/lulesh." + thr + ".x"})
    elif options.algo == 6:
        print("minighost\n")
        ariel.addParams({"executable" : "/home/afrodri/bench2/miniGhost." + thr + ".x"})

    coreCounter = 0
    return ariel

def doQuad(quad, cores, rtr, rtrPort, netAddr):
    sst.pushNamePrefix("q%d"%quad)

    bus = sst.Component("membus", "memHierarchy.Bus")
    bus.addParams({
        "bus_frequency" : clock,
        "bus_latency_cycles" : 1,
        })

    for x in range(cores):
        core = 4*quad + x
        # make the core
        if (useAriel == 0):
            coreObj = sst.Component("cpu_%d"%core,"memHierarchy.streamCPU")
            coreObj.addParams(cpuParams)
        # make l1
        l1id = sst.Component("l1cache_%d"%core, "memHierarchy.Cache")
        l1id.addParams({
            "coherence_protocol": coherence_protocol,
            "cache_frequency": clock,
            "replacement_policy": "lru",
            "cache_size": "8KB",
            "associativity": 8,
            "cache_line_size": 64,
            "low_network_links": 1,
            "access_latency_cycles": 2,
            "L1": 1,
            "statistics": 1,
            "debug": memDebug,
            "debug_level" : 6,
            })
        l1id.addParams(l1PrefetchParams)
        #connect L1 & Core
        if useAriel:
            arielL1Link = sst.Link("core_cache_link_%d"%core)
            portName = "cache_link_" + str(coreCtr.nextPort())
            arielL1Link.connect((ariel, portName,
                                 busLat), 
                                (l1id, "high_network_0", busLat))
        else:
            coreL1Link = sst.Link("core_cache_link_%d"%core)
            coreL1Link.connect((coreObj, "mem_link", busLat),
                               (l1id, "high_network_0", busLat))
        membusLink = sst.Link("cache_bus_link_%d"%core)
        membusLink.connect((l1id, "low_network_0", busLat), (bus, "high_network_%d"%x, busLat))

    #make the L2 for the quad cluster
    l2 = sst.Component("l2cache_nid%d"%netAddr, "memHierarchy.Cache")
    l2.addParams({
        "coherence_protocol": coherence_protocol,
        "cache_frequency": l2clock,
        "replacement_policy": "lru",
        "cache_size": "128KB",
        "associativity": 16,
        "cache_line_size": 64,
        "access_latency_cycles": 23,
        "low_network_links": 1,
        "high_network_links": 1,
        "mshr_num_entries" : 4096, #64,   # TODO: Cesar will update
        "L1": 0,
        "directory_at_next_level": 1,
        "network_address": netAddr,
        "network_bw": coreNetBW,
        "statistics": 1,
        "debug_level" : 6,
        "debug": memDebug
        })
    l2.addParams(l2PrefetchParams)
    link = sst.Link("l2cache_%d_link"%quad)
    link.connect((l2, "high_network_0", busLat), (bus, "low_network_0", busLat))
    link = sst.Link("l2cache_%d_netlink"%quad)
    link.connect((l2, "directory", netLat), (rtr, "port%d"%(rtrPort), netLat))

    sst.popNamePrefix()

# make a cube
def doVS(num, cpu) :
    sst.pushNamePrefix("cube%d"%num)
    ll = sst.Component("logicLayer", "VaultSimC.logicLayer")
    ll.addParams ({
            "clock" : """500Mhz""",
            "vaults" : str(vaultsPerCube),
            "llID" : """0""", 
            "bwlimit" : """32""",
            "LL_MASK" : """0""",
            "terminal" : """1"""
            })
    ll.enableStatistics(["BW_recv_from_CPU"], 
                        {"type":"sst.AccumulatorStatistic",
                         "rate":"0 ns"})
    fromCPU = sst.Link("link_cpu_cube");
    fromCPU.connect((cpu, "cube_link", ccLat),
                    (ll, "toCPU", ccLat))
    #make vaults
    for x in range(0, vaultsPerCube):
        v = sst.Component("ll.Vault"+str(x), "VaultSimC.VaultSimC")
        v.addParams({
                "clock" : """750Mhz""",
                "VaultID" : str(x),
                "numVaults2" : math.log(vaultsPerCube,2)
                })
        v.enableStatistics(["Mem_Outstanding"], 
                           {"type":"sst.AccumulatorStatistic",
                            "rate":"0 ns"})
        ll2V = sst.Link("link_ll_vault" + str(x))
        ll2V.connect((ll,"bus_" + str(x), "1ns"),
                     (v, "bus", "1ns"))
    sst.popNamePrefix()

def doFakeDC(rtr, nextPort, netAddr, dcNum):
    memory = sst.Component("fake_memory", "memHierarchy.MemController")
    memory.addParams({
            "coherence_protocol": coherence_protocol,
            "rangeStart": 0,
            "backend" : "memHierarchy.simpleMem",
            "backend.mem_size": str(rank_size) + "MiB",
            "clock": memclock,
            "statistics": 1,
            "debug": memDebug
            })
    # use a fixed latency
    memory.addParams(memParams)
    # add fake DC
    dc = sst.Component("dc_nid%d"%netAddr, "memHierarchy.DirectoryController")
    print("DC nid%d\n %x to %x\n iSize %x iStep %x" % (netAddr, 0, 0, 0, 0))
    dc.addParams({
            "coherence_protocol": coherence_protocol,
            "network_bw": memNetBW,
            "addr_range_start": 0,
            "addr_range_end":  0,
            "interleave_size": 0/1024,    # in KB
            "interleave_step": 0,         # in KB
            "entry_cache_size": 128*1024, #Entry cache size of mem/blocksize
            "clock": memclock,
            "debug": memDebug,
            "statistics": 1,
            "network_address": netAddr
            })
    #wire up
    memLink = sst.Link("fake_mem%d_link"%dcNum)
    memLink.connect((memory, "direct_link", busLat), (dc, "memory", busLat))
    netLink = sst.Link("fake_dc%d_netlink"%dcNum)
    netLink.connect((dc, "network", netLat), (rtr, "port%d"%(nextPort), netLat))

def doDC(rtr, nextPort, netAddr, dcNum):
    start_pos = (dcNum * interleave_size);
    interleave_step = PIMs*(interleave_size/1024) # in KB
    end_pos = start_pos + ((512*1024*1024)-(interleave_size*(PIMs-1))) 

    # add memory
    memory = sst.Component("memory", "memHierarchy.MemController")
    memory.addParams({
            "coherence_protocol": coherence_protocol,
            "rangeStart": start_pos,
            "backend.mem_size": rank_size,
            "clock": memclock,
            "statistics": 1,
            "debug": memDebug
            })
    if (useVaultSim == 1):
        # use vaultSim
        memory.addParams({"backend" : "memHierarchy.vaultsim"})
        doVS(dcNum, memory)
    else:
        # use a fixed latency
        memory.addParams(memParams)

    # add DC
    dc = sst.Component("dc_nid%d"%netAddr, "memHierarchy.DirectoryController")
    print("DC nid%d\n %x to %x\n iSize %x iStep %x" % (netAddr, start_pos, end_pos, interleave_size, interleave_step))
    dc.addParams({
            "coherence_protocol": coherence_protocol,
            "network_bw": memNetBW,
            "addr_range_start": start_pos,
            "addr_range_end":  end_pos,
            "interleave_size": interleave_size/1024,    # in KB
            "interleave_step": interleave_step,         # in KB
            "entry_cache_size": 128*1024, #Entry cache size of mem/blocksize
            "clock": memclock,
            "debug": memDebug,
            "statistics": 1,
            "network_address": netAddr
            })
    #wire up
    memLink = sst.Link("mem%d_link"%dcNum)
    memLink.connect((memory, "direct_link", busLat), (dc, "memory", busLat))
    netLink = sst.Link("dc%d_netlink"%dcNum)
    netLink.connect((dc, "network", netLat), (rtr, "port%d"%(nextPort), netLat))

def doCPU(): 
    sst.pushNamePrefix("cpu")
    # make the router
    rtr = sst.Component("cpuRtr", "merlin.hr_router")
    rtr.addParams(routerParams)
    rtr.addParams({"id" : 0})
    nextPort = 2 #0,1 are reserved for router-to-router
    nextAddr = 0 # Merlin-level network address

    #make the quads
    for x in range(CPUQuads):
        doQuad(x, 4, rtr, nextPort, nextAddr)
        nextPort += 1
        nextAddr += 1

    #fake DCs
    for x in range(fakeCPU_DC):
        doFakeDC(rtr, nextPort, nextAddr, -1)
        nextPort += 1
        nextAddr += 1

    sst.popNamePrefix()
    return rtr

def doPIM(pimNum, prevRtr):
    sst.pushNamePrefix("pim%d"%(pimNum))
    #make the router
    rtr = sst.Component("pimRtr" + str(pimNum), "merlin.hr_router")
    rtr.addParams(routerParams)
    rtr.addParams({"id" : pimNum+1})
    nextPort = 2 #0,1 are reserved for router-to-router
    nextAddr = (pimNum+1)*localPorts # Merlin-level network address

    #make the quads
    for x in range(PIMQuads):
        doQuad(x, 4, rtr, nextPort, nextAddr)
        nextPort += 1
        nextAddr += 1

    # real DC
    doDC(rtr, nextPort, nextAddr, pimNum)
    nextPort += 1
    nextAddr += 1

    #fake DCs
    for x in range(fakePIM_DC):
        doFakeDC(rtr, nextPort, nextAddr, pimNum)
        nextPort += 1
        nextAddr += 1

    # connect to chain
    wrapLink = sst.Link("p%d"%pimNum)
    wrapLink.connect((prevRtr,"port0", netLat),
                     (rtr, "port1", netLat))
    
    sst.popNamePrefix()
    return rtr
        

# "MAIN"

# Define SST core options
#sst.setProgramOption("partitioner", "self")
sst.setProgramOption("stopAtCycle", "300 us")
sst.setStatisticLoadLevel(7)   
sst.setStatisticOutput("sst.statOutputConsole")

#if needed, create the ariel component
if useAriel:
    ariel = makeAriel()

#make the CPU
cpuRtr = doCPU()

#make the PIMs
prevRtr = cpuRtr
for x in range(PIMs):
    prevRtr = doPIM(x, prevRtr)

# complete the torus
wrapLink = sst.Link("wrapLink")
wrapLink.connect((prevRtr,"port0", netLat),
                 (cpuRtr, "port1", netLat))
