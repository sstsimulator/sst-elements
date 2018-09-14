import sst

num_cpu = 2
num_mem = 2
mem_size = 0x40000
niter = 1000
netBW = "80GiB/s"
debug = 2
debug_level = 0


# CPU0 -> L1 -> Net0
# CPU1 -> L1 -> Net0
# Net0 -> Bridge -> Net1
# Mem0 -> Net1
# Mem1 -> Net1


class Network:
    def __init__(self, name):
        self.name = name
        self.ports = 0
        self.rtr = sst.Component("rtr_%s"%name, "merlin.hr_router")
        self.rtr.addParams({
            "id": 0,
            "topology": "merlin.singlerouter",
            "link_bw" : netBW,
            "xbar_bw" : netBW,
            "flit_size" : "8B",
            "input_latency" : "10ns",
            "output_latency" : "10ns",
            "input_buf_size" : "1KB",
            "output_buf_size" : "1KB",
            "debug" : 1,
            })


    def getNextPort(self):
        self.ports += 1
        self.rtr.addParam("num_ports", self.ports)
        return (self.ports-1)


def buildCPU(num, network):
    netPort = network.getNextPort()

    cpu = sst.Component("cpu_%d"%num, "memHierarchy.streamCPU")
    cpu.addParams({
        "commFreq": 1,
        "memSize": mem_size - 1,
        "num_loadstore": niter,
        })

    l1 = sst.Component("l1_%d"%num, "memHierarchy.Cache")
    l1.addParams({
        "debug": debug,
        "debug_level" : debug_level,
        "cache_size": "2KiB",
        "access_latency_cycles" : 4,
        "cache_frequency": "1GHz",
        "associativity" : 4,
        "cache_line_size": 64,
        "L1": 1,
        "network_bw" : netBW,
        })

    cpuLink = sst.Link("cpu-cache-%d"%num)
    cpuLink.connect( (cpu, "mem_link", "500ps"), (l1, "high_network_0", "500ps"))

    rtrLink = sst.Link("L1-net-%d"%num)
    rtrLink.connect( (l1, "directory", "500ps"), (network.rtr, "port%d"%netPort, "500ps") )


def buildMem(num, network):
    netPort = network.getNextPort()

    mem = sst.Component("mem%d"%num, "memHierarchy.MemController")
    mem.addParams({
        "debug": debug,
        "debug_level" : debug_level,
        "backend" : "memHierarchy.simpleMem",
        "backend.mem_size" : 1,
        "clock" : "1GHz"
        })

    dc = sst.Component("dc%d"%num, "memHierarchy.DirectoryController")
    dc.addParams({
        "debug": debug,
        "debug_level" : debug_level,
        "entry_cache_size": 256*1024*1024, #Entry cache size of mem/blocksize
        "clock": "1GHz",
        "network_bw": netBW,
        "addr_range_start" : num * (mem_size / num_mem),
        "addr_range_end" : (num+1) * (mem_size / num_mem) -1,
        })

    memLink = sst.Link("MemDir_%d"%num)
    memLink.connect( (dc, "memory", "500ps"), (mem, "direct_link", "500ps") )
    dcLink = sst.Link("DCNet%d"%num)
    dcLink.connect( (dc, "network", "500ps"), (network.rtr, "port%d"%netPort, "500ps") )




def bridge(net0, net1):
    net0port = net0.getNextPort()
    net1port = net1.getNextPort()
    name = "%s-%s"%(net0.name, net1.name)
    bridge = sst.Component("Bridge:%s"%name, "merlin.Bridge")
    bridge.addParams({
        "translator": "memHierarchy.MemNetBridge",
        "debug": debug,
        "debug_level" : debug_level,
        "network_bw" : netBW,
    })
    link = sst.Link("B0-%s"%name)
    link.connect( (bridge, "network0", "500ps"), (net0.rtr, "port%d"%net0port, "500ps") )
    link = sst.Link("B1-%s"%name)
    link.connect( (bridge, "network1", "500ps"), (net1.rtr, "port%d"%net1port, "500ps") )


# Network 0
net0 = Network("CPU_Net")
for cpu in range(num_cpu):
    buildCPU(cpu, net0)

# Network 1
net1 = Network("MEM_Net")
for mem in range(num_mem):
    buildMem(mem, net1)

net2 = Network("Middle_Net")

bridge(net0, net2)
bridge(net1, net2)


sst.setStatisticLoadLevel(16)
sst.enableAllStatisticsForAllComponents({"type": "sst.AccumulatorStatistic"})
sst.setStatisticOutput("sst.statOutputCSV")
sst.setStatisticOutputOptions({
    "filepath" : "stats.csv",
    "separator" : ", ",
    })
