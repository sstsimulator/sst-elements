# Automatically generated SST Python input
import sst
from mhlib import componentlist

# Test timingDRAM with transactionQ = reorderTransactionQ and AddrMapper=sandyBridgeAddrMapper and pagepolicy=timeoutPagePolicy

# Define the simulation components
cpu_params = {
    "clock" : "3GHz",
    "do_write" : 1,
    "num_loadstore" : "5000",
    "memSize" : "0x100000"
        }

bus = sst.Component("bus", "memHierarchy.Bus")
bus.addParams({ "bus_frequency" : "2Ghz" })


l3cache = sst.Component("l3cache", "memHierarchy.Cache")
l3cache.addParams({
      "access_latency_cycles" : "30",
      "mshr_latency_cycles" : 3,
      "cache_frequency" : "2Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "16",
      "cache_line_size" : "64",
      "cache_size" : "64 KB",
      "debug" : "0",
      "verbose" : 2,
})
l3tol2 = l3cache.setSubComponent("cpulink", "memHierarchy.MemLink")
l3NIC = l3cache.setSubComponent("memlink", "memHierarchy.MemNIC")
l3NIC.addParams({
    "group" : 1,
    "network_bw" : "25GB/s",
})

for i in range(0,8):
    cpu = sst.Component("cpu" + str(i), "memHierarchy.trivialCPU")
    cpu.addParams(cpu_params)
    rngseed = i * 12
    cpu.addParams({
        "rngseed" : rngseed,
        "commFreq" : (rngseed % 7) + 1 })
    
    iface = cpu.setSubComponent("memory", "memHierarchy.memInterface")
    
    l1cache = sst.Component("c" + str(i) + ".l1cache", "memHierarchy.Cache")
    l1cache.addParams({
        "access_latency_cycles" : "4",
        "cache_frequency" : "2Ghz",
        "replacement_policy" : "lru",
        "coherence_protocol" : "MESI",
        "associativity" : "4",
        "cache_line_size" : "64",
        "cache_size" : "4 KB",
        "L1" : "1",
        "verbose" : 2,
        "debug" : "0"
        })

    l2cache = sst.Component("c" + str(i) + ".l2cache", "memHierarchy.Cache")
    l2cache.addParams({
      "access_latency_cycles" : "9",
      "mshr_latency_cycles" : 2,
      "cache_frequency" : "2Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "32 KB",
      "verbose" : 2,
      "debug" : "0"
    })

    # Connect
    link_cpu_l1 = sst.Link("link_cpu_l1_" + str(i))
    link_cpu_l1.connect( (iface, "port", "500ps"), (l1cache, "high_network_0", "500ps") )

    link_l1_l2 = sst.Link("link_l1_l2_" + str(i))
    link_l1_l2.connect( (l1cache, "low_network_0", "500ps"), (l2cache, "high_network_0", "500ps") )

    link_l2_bus = sst.Link("link_l2_bus_" + str(i))
    link_l2_bus.connect( (l2cache, "low_network_0", "1000ps"), (bus, "high_network_" + str(i), "1000ps") )


comp_chiprtr = sst.Component("chiprtr", "merlin.hr_router")
comp_chiprtr.addParams({
      "xbar_bw" : "1GB/s",
      "link_bw" : "1GB/s",
      "input_buf_size" : "1KB",
      "num_ports" : "2",
      "flit_size" : "72B",
      "output_buf_size" : "1KB",
      "id" : "0",
      "topology" : "merlin.singlerouter"
})
comp_chiprtr.setSubComponent("topology","merlin.singlerouter")
dirctrl = sst.Component("dirctrl", "memHierarchy.DirectoryController")
dirctrl.addParams({
      "coherence_protocol" : "MESI",
      "debug" : "0",
      "verbose" : 2,
      "entry_cache_size" : "32768",
    "addr_range_end" : "0x1F000000",
    "addr_range_start" : "0x0"
})
dirtoM = dirctrl.setSubComponent("memlink", "memHierarchy.MemLink")
dirNIC = dirctrl.setSubComponent("cpulink", "memHierarchy.MemNIC")
dirNIC.addParams({
    "group" : 2,
    "network_bw" : "25GB/s",
})
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
    "backing" : "none",
    "verbose" : 2,
    "clock" : "1.2GHz",
    "debug" : 0,
    "debug_level" : 5,
    "addr_range_end" : 512*1024*1024-1,
})
memory = memctrl.setSubComponent( "backend", "memHierarchy.timingDRAM")
memory.addParams({
    "id" : 0,
    "addrMapper" : "memHierarchy.sandyBridgeAddrMapper",
    "addrMapper.interleave_size" : "64B",
    "addrMapper.row_size" : "1KiB",
    "clock" : "1.2GHz",
    "mem_size" : "512MiB",
    "channels" : 1,
    "channel.numRanks" : 2,
    "channel.rank.numBanks" : 16,
    "channel.transaction_Q_size" : 32,
    "channel.rank.bank.CL" : 14,
    "channel.rank.bank.CL_WR" : 12,
    "channel.rank.bank.RCD" : 14,
    "channel.rank.bank.TRP" : 14,
    "channel.rank.bank.dataCycles" : 2,
    "channel.rank.bank.pagePolicy" : "memHierarchy.timeoutPagePolicy",
    "channel.rank.bank.transactionQ" : "memHierarchy.reorderTransactionQ",
    "channel.rank.bank.pagePolicy.timeoutCycles" : 50,
    "printconfig" : 0,
    "channel.printconfig" : 0,
    "channel.rank.printconfig" : 0,
    "channel.rank.bank.printconfig" : 0,
})

# Do lower memory hierarchy links
link_bus_l3 = sst.Link("link_bus_l3")
link_bus_l3.connect( (bus, "low_network_0", "500ps"), (l3tol2, "port", "500ps") )

link_l3_net = sst.Link("link_l3_net")
link_l3_net.connect( (l3NIC, "port", "10000ps"), (comp_chiprtr, "port1", "2000ps") )
link_dir_net = sst.Link("link_dir_net")
link_dir_net.connect( (comp_chiprtr, "port0", "2000ps"), (dirNIC, "port", "2000ps") )
link_dir_mem = sst.Link("link_dir_mem")
link_dir_mem.connect( (dirtoM, "port", "10000ps"), (memctrl, "direct_link", "10000ps") )

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)

