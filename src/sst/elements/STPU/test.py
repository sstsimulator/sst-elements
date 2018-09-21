import sst

# Define the simulation components
comp_stpu = sst.Component("STPU", "STPU.STPU")
comp_stpu.addParams({
    "verbose" : 1,
    "clock" : "1GHz",
    "BWPperTic" : 1,
    "STSDispatch" : 1,
    "STSParallelism" : 1
})

comp_memory = sst.Component("memory", "memHierarchy.MemController")
comp_memory.addParams({
      "debug" : 1,
      "debug_level" : 10,
      "backend.access_time" : "100 ns",
      "backing" : "malloc", 
      "clock" : "1GHz",
      "backend.mem_size" : "512MiB"
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
#sst.enableAllStatisticsForComponentType("memHierarchy.MemController")


# Define the simulation links
link_stpu_mem = sst.Link("link_stpu_mem")
link_stpu_mem.connect( (comp_stpu, "mem_link", "1000ps"), (comp_memory, "direct_link", "1000ps") )

