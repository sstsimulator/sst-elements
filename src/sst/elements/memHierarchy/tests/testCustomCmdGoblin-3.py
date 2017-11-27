import sst
import sys
import ConfigParser, argparse
from utils import *


# Parse commandline arguments
parser = argparse.ArgumentParser()
parser.add_argument("-c", "--config", help="specify configuration file", required=False)
parser.add_argument("-v", "--verbose", help="increase verbosity of output", action="store_true")
parser.add_argument("-l", "--statlevel", help="statistics level", type=int, default=16)

args = parser.parse_args()

verbose = args.verbose
cfgFile = "miranda.cfg"
statLevel = args.statlevel

# Build Configuration Information
config = Config(cfgFile, verbose=verbose)

print "Configuring Network-on-Chip..."

router = sst.Component("router", "merlin.hr_router")
router.addParams(config.getRouterParams())
router.addParam('id', 0)

# Connect Cores & caches
for next_core_id in range(config.total_cores):
    print "Configuring core %d..."%next_core_id

    cpu = sst.Component("cpu%d"%(next_core_id), "miranda.BaseCPU")
    cpu.addParams(config.getCoreConfig(next_core_id))

    l1 = sst.Component("l1cache_%d"%(next_core_id), "memHierarchy.Cache")
    l1.addParams(config.getL1Params())

    l2 = sst.Component("l2cache_%d"%(next_core_id), "memHierarchy.Cache")
    l2.addParams(config.getL2Params())

    connect("cpu_cache_link_%d"%next_core_id,
            cpu, "cache_link",
            l1, "high_network_0",
            config.ring_latency).setNoCut()

    connect("l2cache_%d_link"%next_core_id,
            l1, "low_network_0",
            l2, "high_network_0",
            config.ring_latency).setNoCut()

    connect("l2_ring_link_%d"%next_core_id,
            l2, "directory",
            router, "port%d"%next_core_id,
            config.ring_latency)

# Connect Memory and Memory Controller to the ring
mem = sst.Component("memory", "memHierarchy.CoherentMemController")
mem.addParams(config.getMemParams())

dc = sst.Component("dc", "memHierarchy.DirectoryController")
dc.addParams(config.getDCParams(0))

connect("mem_link_0",
        mem, "direct_link",
        dc, "memory",
        config.ring_latency)

connect("dc_link_0",
        dc, "network",
        router, "port%d"%config.total_cores,
        config.ring_latency)

# ===============================================================================

# Enable SST Statistics Outputs for this simulation
sst.setStatisticLoadLevel(statLevel)
sst.enableAllStatisticsForAllComponents({"type":"sst.AccumulatorStatistic"})

sst.setStatisticOutput("sst.statOutputConsole")

print "Completed configuring the EX3 model"
