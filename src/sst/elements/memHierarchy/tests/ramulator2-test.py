import sst
#from mhlib import componentlist
debug_mem=1
 
# Define the simulation components
cpu0 = sst.Component("core0", "memHierarchy.standardCPU")
cpu0.addParams({
    "memFreq" : 1,
    "memSize" : "100KiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "rngseed" : 989,
    "maxOutstanding" : 64,
    "opCount" : 10,
    "reqsPerIssue" : 4,
    "write_freq" : 25, # 25% writes
    "read_freq" : 75,  # 75% reads
    "debug" : 1,
    "debug_level" : 10
})
iface0 = cpu0.setSubComponent("memory", "memHierarchy.standardInterface")

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : "1",
    "debug_level" : "10",
    "clock" : "1GHz",
    "addr_range_end" : 100*1024-1,
})

memory = memctrl.setSubComponent("backend", "memHierarchy.ramulator2")
memory.addParams({
      "mem_size" : "512MiB",
      "configFile" : "ramulator2-ddr4.cfg",
      "debug_level" : "10",
      "debug" : "1"
})

# Enable statistics
#sst.setStatisticLoadLevel(7)
#sst.setStatisticOutput("sst.statOutputConsole")
#for a in componentlist:
#    sst.enableAllStatisticsForComponentType(a)
 
# Define the simulation links
link_cpu0_l1cache_link = sst.Link("link_cpu0_l1cache_link")
link_cpu0_l1cache_link.connect( (iface0, "port", "1000ps"), (memctrl, "direct_link", "1000ps") )