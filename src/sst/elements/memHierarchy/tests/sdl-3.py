import sst
from mhlib import componentlist

DEBUG_L1 = 0
DEBUG_MEM = 0
DEBUG_LEVEL = 10

# Define the simulation components
comp_cpu = sst.Component("cpu", "memHierarchy.trivialCPU")
comp_cpu.addParams({
      "do_write" : "1",
      "num_loadstore" : "1000",
      "commFreq" : "100",
      "memSize" : "0x1000"
})
iface = comp_cpu.setSubComponent("memory", "memHierarchy.memInterface")

l1cache = sst.Component("l1cache", "memHierarchy.Cache")
l1cache.addParams({
      "access_latency_cycles" : "4",
      "cache_frequency" : "2 Ghz",
      "coherence_protocol" : "MSI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "debug" : DEBUG_L1,
      "debug_level" : DEBUG_LEVEL,
      "L1" : "1",
      "cache_size" : "4KiB"
})
# Replacement policy - can declare here or as a parameter (only if part of memHierarchy's core set of policies and using default parameters)
l1cache.setSubComponent("replacement", "memHierarchy.replacement.lru")

# Links to core & mem
l1toC = l1cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l1toM = l1cache.setSubComponent("memlink", "memHierarchy.MemLink")

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "debug" : "0",
    "clock" : "1GHz",
    "request_width" : "64",
    "addr_range_end" : 512*1024*1024-1,
})
mtol1 = memctrl.setSubComponent("cpulink", "memHierarchy.MemLink")

memory = memctrl.setSubComponent("backend", "memHierarchy.timingDRAM")
memory.addParams({
    "mem_size" : "512MiB",
    "id" : 0,
    "addrMapper" : "memHierarchy.roundRobinAddrMapper",
    "addrMapper.interleave_size" : "64B",
    "addrMapper.row_size" : "1KiB",
    "channels" : 3,
    "channel.numRanks" : 3,
    "channel.rank.numBanks" : 5,
    "channel.transaction_Q_size" : 32,
    "channel.rank.bank.CL" : 14,
    "channel.rank.bank.CL_WR" : 12,
    "channel.rank.bank.RCD" : 14,
    "channel.rank.bank.TRP" : 14,
    "channel.rank.bank.dataCycles" : 2,
    "channel.rank.bank.pagePolicy" : "memHierarchy.simplePagePolicy",
    "channel.rank.bank.transactionQ" : "memHierarchy.reorderTransactionQ",
    "printconfig" : 0,
    "channel.printconfig" : 0,
    "channel.rank.printconfig" : 0,
    "channel.rank.bank.printconfig" : 0,
})
memory.setSubComponent("transactionQ", "memHierarchy.reorderTransactionQ")
pp = memory.setSubComponent("pagePolicy", "memHierarchy.simplePagePolicy")
pp.addParams({"close" : 0 })

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)


# Define the simulation links
link_cpu_cache = sst.Link("link_cpu_cache")
link_cpu_cache.connect( (iface, "port", "1000ps"), (l1toC, "port", "1000ps") )
link_mem_bus = sst.Link("link_mem_bus")
link_mem_bus.connect( (l1toM, "port", "50ps"), (mtol1, "port", "50ps") )
