import sst
import os
from optparse import OptionParser

# options
op = OptionParser()
cwd = os.path.dirname(__file__)
op.add_option("-n", "--neurons", action="store", type="string", dest="neurons", default=cwd+"/model")
op.add_option("-d", "--dt", action="store", type="float", dest="dt", default="1")
op.add_option("-l", "--steps", action="store", type="int", dest="steps", default="1000")
# cache size in KiB
op.add_option("-c", "--cacheSz", action="store", type="int", dest="cacheSz", default=2)
#sts dispatch & parallelism
op.add_option("-s", "--STS", action="store", type="int", dest="sts", default=4)
# max memory out
op.add_option("-m", "--memOut", action="store", type="int", dest="memOut", default=4)
(options, args) = op.parse_args()

# Define the simulation components
comp_gna = sst.Component("GNA", "GNA.core")
comp_gna.addParams({
    "verbose" : 1,
    "modelPath" : options.neurons,
    "dt" : options.dt,
    "steps" : options.steps,
    "clock" : "1GHz",
    "InputsPerTic" : 1,
    "STSDispatch" : options.sts,
    "STSParallelism" : options.sts,
    "MaxOutMem" : options.memOut 
})

comp_memctrl = sst.Component("memory", "memHierarchy.MemController")
comp_memctrl.addParams({
      "debug" : 0,
      "debug_level" : 10,
      "backing" : "malloc", 
      "clock" : "1GHz",
      "addr_range_start" : 0,
})
memory = comp_memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
    "mem_size" : "512MiB",
    "access_time" : "200 ns",
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
sst.enableAllStatisticsForComponentType("memHierarchy.Cache")


# Define the simulation links
link_gna_cache = sst.Link("link_gna_mem")
link_gna_cache.connect( (comp_gna, "mem_link", "50ps"), (comp_memctrl, "direct_link", "50ps") )
