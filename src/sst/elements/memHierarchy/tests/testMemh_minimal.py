import sst
    
# Define some output/debug parameters
verbose = 2
DEBUG_MEM = 0
DEBUG_LEVEL = 10
 
# Minimal test: A cpu + memory
cpu = sst.Component("core", "memHierarchy.standardCPU")
cpu.addParams({
    "memFreq" : 2,
    "memSize" : "4KiB",
    "verbose" : 0,
    "clock" : "3.5GHz",
    "rngseed" : 111,
    "maxOutstanding" : 16,
    "opCount" : 2500, # Number of operations to do before exiting. Increase to increase simulation time.
    "reqsPerIssue" : 3,
    "write_freq" : 40, # 40% writes
    "read_freq" : 60,  # 60% reads
})  
                     
iface = cpu.setSubComponent("memory", "memHierarchy.standardInterface")
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : DEBUG_MEM,
    "debug_level" : DEBUG_LEVEL,
    "clock" : "1GHz",
    "verbose" : verbose,
    "addr_range_end" : 512*1024*1024-1,
})  

memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
    "access_time" : "1000ns",
    "mem_size" : "512MiB"
})  

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
sst.enableAllStatisticsForAllComponents()
                                  
# Define the simulation links
link_cpu_mem = sst.Link("link_cpu_mem")
link_cpu_mem.connect( (iface, "port", "1000ps"), (memctrl, "direct_link", "1000ps") )
