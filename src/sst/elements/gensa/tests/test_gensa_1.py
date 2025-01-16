import sst
import os
from optparse import OptionParser

# options
op = OptionParser()
cwd = os.path.dirname(__file__)
op.add_option("-n", "--neurons", action="store", type="string", dest="neurons", default=cwd+"/model")
op.add_option("-d", "--dt", action="store", type="float", dest="dt", default="1")
op.add_option("-l", "--steps", action="store", type="int", dest="steps", default="20")
(options, args) = op.parse_args()


# Define the simulation components

core = sst.Component("gensa", "gensa.core")
core.addParams({
    "verbose"   : 1,
    "modelPath" : options.neurons,
    "dt"        : options.dt,
    "steps"     : options.steps,
    "clock"     : "1GHz"
})

memoryController = sst.Component("memory", "memHierarchy.MemController")
memoryController.addParams({
      "debug"            : 0,
      "debug_level"      : 10,
      "backing"          : "malloc", 
      "clock"            : "1GHz",
      "addr_range_start" : 0,
})
memory = memoryController.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
    "mem_size"    : "512MiB",
    "access_time" : "10ns"
})

link_core_memory = sst.Link("link_core_memory")
link_core_memory.connect( (core, "mem_link", "50ps"), (memoryController, "highlink", "50ps") )
link_core_memory.setNoCut()  # put memory on same partition with core

msg_size        = "8B"
link_bw         = "32GB/s"
flit_size       = "8B"
input_buf_size  = "64B"
output_buf_size = "64B"

rtr = sst.Component("rtr", "merlin.hr_router")
rtr.addParams({
    "id"              : 0,
    "num_ports"       : 1,
    "flit_size"       : flit_size,
    "xbar_bw"         : link_bw,
    "link_bw"         : link_bw,
    "input_buf_size"  : input_buf_size,
    "output_buf_size" : output_buf_size
})
rtr.setSubComponent("topology", "merlin.singlerouter")

networkIF = core.setSubComponent("networkIF", "merlin.linkcontrol")
networkIF.addParams({
    "link_bw": "1GB/s"
})

link_IF_rtr = sst.Link("link_IF_rtr")
link_IF_rtr.connect( (networkIF, "rtr_port", "50ps"), (rtr, "port0", "50ps") )
link_IF_rtr.setNoCut()  # put network interface on same partition with core



# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
sst.enableAllStatisticsForComponentType("memHierarchy.Cache")

