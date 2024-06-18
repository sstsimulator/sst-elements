import sst
import sys
import os
import params
import argparse

# The CoreGroup class builds a single core group. It needs a group_id as input as well as
# network connections. It will build a L1 cache for each core, an L2, and a
# memory controller. All of this will be attached to the network.
# Right now, all the links have the same latency.
class CoreGroup:
    def __init__(self, group_id, cores_per_group, ngroup, net, latency):
        # Make L1-L2 bus
        bus = sst.Component("L1L2Bus", "memHierarchy.Bus")
        bus.addParams(params.bus)

        # Make L1s, connect to bus
        for i in range(cores_per_group):

            sst.pushNamePrefix(f"Core{i}")

            core = sst.Component("Core", "miranda.BaseCPU")
            core.addParams(params.core)

            gen = core.setSubComponent("generator", "miranda.GEMMGenerator")
            gen.addParams({"matrix_M": 128, "matrix_N": 128, "matrix_K": 64, "b_ptr": 0x40000000,
                           "num_threads": cores_per_group*ngroup, "thread_id": group_id*cores_per_group+i})

            l1 = sst.Component("L1Cache", "memHierarchy.Cache")
            l1.addParams(params.l1)

            core_to_l1 = sst.Link("core_to_l1")
            l1_to_bus  = sst.Link("l1_to_bus")

            core_to_l1.connect(
                (core, "cache_link", latency),
                (l1, "high_network_0", latency))

            l1_to_bus.connect(
                (l1, "low_network_0", latency ),
                (bus, f"high_network_{i}", latency))
            sst.popNamePrefix()

        # Make L2, attach to the bus to the l1
        params.l2["slide_id"] = group_id*cores_per_group+i

        l2 = sst.Component("L2Cache", "memHierarchy.Cache")
        l2.addParams(params.l2)

        l2cpulink = l2.setSubComponent("cpulink", "memHierarchy.MemLink")
        l2cpulink.addParams(params.memlink)

        bus_to_l2 = sst.Link("bus_to_l2")

        bus_to_l2.connect(
            (bus, "low_network_0", latency),
            (l2cpulink, "port", latency))

        # Attach L2 to network
        l2nic = l2.setSubComponent("memlink", "memHierarchy.MemNIC")
        l2nic.addParams(params.l2nic)

        l2link = l2nic.setSubComponent("linkcontrol", "kingsley.linkcontrol")
        l2link.addParams(params.linkcontrol)

        l2_to_net = sst.Link("l2_to_net")
        l2_to_net.connect(
            (l2link, "rtr_port", latency),
            (net, "local0", latency)
        )

        # Make Directory, link to network
        dc = sst.Component("Directory", "memHierarchy.DirectoryController")
        dc.addParams(params.dc[group_id])

        dcnic = dc.setSubComponent("cpulink", "memHierarchy.MemNIC")
        dcnic.addParams(params.dcnic)

        dclink = dcnic.setSubComponent("linkcontrol", "kingsley.linkcontrol")
        dclink.addParams(params.linkcontrol)

        dc2mem = dc.setSubComponent("memlink", "memHierarchy.MemLink")
        dc2mem.addParams(params.memlink)

        net_to_dclink = sst.Link("net_to_dclink")
        net_to_dclink.connect(
            (net,    "local1",   latency),
            (dclink, "rtr_port", latency))

        # Memory controller, linked to dc
        memctrl = sst.Component("MemoryController", "memHierarchy.MemController")
        memctrl.addParams(params.memctrl[group_id])
        memory = memctrl.setSubComponent("backend", "memHierarchy.simpleDRAM")
        memory.addParams(params.dram)

        dc2mem_to_memctrl = sst.Link("dc2mem_to_memctrl")
        dc2mem_to_memctrl.connect(
            (dc2mem, "port",        latency),
            (memctrl, "direct_link", latency))

# Parse arguments
parser = argparse.ArgumentParser(description='Run a Fujitsu A64FX-like processor simulation',
                                 formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument('-n', '--ncpu',     default='1', help='Number of cpus in each core group.', type=int)
parser.add_argument('-r', '--nrows',   default='2', help='Number of rows of core groups.', type=int)
parser.add_argument('-c', '--ncols',   default='2', help='Number of columns of core groups.', type=int)
parser.add_argument('-m', '--im',   action=argparse.BooleanOptionalAction, help='Use interleaved memory')
config = parser.parse_args(sys.argv[1:])

if (config.nrows < 2 or config.ncols < 2):
    print("Error: Kingsley requires at least 2 routers per dimension")
    exit(1)

print('Running with configuration: {}'.format(vars(config)))

# Initialize our Param object
params = params.Param(config.ncpu, config.nrows*config.ncols, config.im)
default_latency = "100ps"

#################
## SIM WIRE UP ##
#################

# Create the grid, row by row. Make the router, then pass it to
# the CoreGroup constructor so the compute and memory can be attached.
# Then connect the router to its north and west neighbors.
grid = {}
for row in range(config.nrows):
    for col in range(config.ncols):
        sst.pushNamePrefix(f'CoreGroup_r{row}c{col}')

        grid[(row, col)] = sst.Component("Router", "kingsley.noc_mesh")
        grid[(row, col)].addParams(params.noc)

        if (row != 0):
            link_ns = sst.Link("link_ns")
            link_ns.connect(
                (grid[(row  , col)], "north", default_latency),
                (grid[(row-1, col)], "south", default_latency))

        if (col != 0):
            link_ew = sst.Link("link_ew")
            link_ew.connect(
                (grid[(row, col  )], "west", default_latency),
                (grid[(row, col-1)], "east", default_latency))

        CoreGroup(row*config.ncols+col, config.ncpu, config.nrows*config.ncols, grid[(row, col)], default_latency)

        sst.popNamePrefix()

# Define SST core options
sst.setProgramOption("timebase", "1ps")
#sst.setProgramOption("stopAtCycle", "0 ns")
sst.setStatisticLoadLevel(9)
sst.enableAllStatisticsForAllComponents()

outfile = "./numa_output_BM.csv"
if config.im:
    outfile = "./numa_output_IM.csv"
print("Writng to " + outfile)
sst.setStatisticOutput("sst.statOutputCSV", {"filepath" : outfile, "separator" : ", " } )

