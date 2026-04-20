# testManagerLogic.py
# Puts a faultInjectorMemH on each L1 cache and assigns each Hali to manage exactly one
# injector via separate PM registries: hali_0 -> l1_0 (c0_l1cache), hali_1 -> l1_1, etc.
# Run with: sst testManagerLogic.py
# Look for "[ManagerLogic]" debug lines to verify each manager only sees its own PM.

import sst
from mhlib import componentlist

DEBUG_L1 = 0
DEBUG_L2 = 0
DEBUG_L3 = 0
DEBUG_DIR = 0
DEBUG_MEM = 0
DEBUG_LEVEL = 0

# Define the simulation components
c0_l1cache = sst.Component("l1cache0.mesi", "memHierarchy.Cache")
hali_0 = sst.Component("hali_0", "Carcosa.Hali")
hali_1 = sst.Component("hali_1", "Carcosa.Hali")
hali_2 = sst.Component("hali_2", "Carcosa.Hali")
hali_3 = sst.Component("hali_3", "Carcosa.Hali")
hali_link0 = sst.Link("hali_link0")
hali_link1 = sst.Link("hali_link1")
hali_link2 = sst.Link("hali_link2")
hali_link3 = sst.Link("hali_link3")
haliCtrl0 = sst.Link("haliCtrl_link0")
haliCtrl1 = sst.Link("haliCtrl_link1")
haliCtrl2 = sst.Link("haliCtrl_link2")
haliCtrl3 = sst.Link("haliCtrl_link3")
halitoCPULink0 = sst.Link("CpuLink0")
halitoCPULink1 = sst.Link("CpuLink1")
halitoCPULink2 = sst.Link("CpuLink2")
halitoCPULink3 = sst.Link("CpuLink3")

# Each Hali has its own PM registry; its FaultInjManager will only see PMs that register with that id.
# debugManagerLogic=True enables [ManagerLogic] and PM-read debug prints.
hali_0.addParams({"verbose": True, "pmRegistryId": "hali_0", "debugManagerLogic": True})
hali_1.addParams({"verbose": True, "pmRegistryId": "hali_1", "debugManagerLogic": True})
hali_2.addParams({"verbose": True, "pmRegistryId": "hali_2", "debugManagerLogic": True})
hali_3.addParams({"verbose": True, "pmRegistryId": "hali_3", "debugManagerLogic": True})

hali_0.addLink(haliCtrl0, "memCtrl", "10ns")
hali_1.addLink(haliCtrl1, "memCtrl", "10ns")
hali_2.addLink(haliCtrl2, "memCtrl", "10ns")
hali_3.addLink(haliCtrl3, "memCtrl", "10ns")

hali_0.addLink(halitoCPULink0, "cpu", "10ns")
hali_1.addLink(halitoCPULink1, "cpu", "10ns")
hali_2.addLink(halitoCPULink2, "cpu", "10ns")
hali_3.addLink(halitoCPULink3, "cpu", "10ns")
hali_0.addLink(hali_link0, "left", "10ns")
hali_0.addLink(hali_link1, "right", "10ns")
hali_1.addLink(hali_link1, "left", "10ns")
hali_1.addLink(hali_link2, "right", "10ns")
hali_2.addLink(hali_link2, "left", "10ns")
hali_2.addLink(hali_link3, "right", "10ns")
hali_3.addLink(hali_link3, "left", "10ns")
hali_3.addLink(hali_link0, "right", "10ns")

cpu0 = sst.Component("core0", "Carcosa.FaultInjCPU")
cpu0.addParams({
    "clock" : "2.2GHz",
    "memFreq" : "4",
    "rngseed" : "101",
    "memSize" : "1MiB",
    "verbose" : 0,
    "clock" : "2GHz",
    "maxOutstanding" : 16,
    "opCount" : 5000,
    "reqsPerIssue" : 4,
    "write_freq" : 40,
    "read_freq" : 60,
})
cpu0.addLink(halitoCPULink0, "haliToCPU", "10ns")
iface0 = cpu0.setSubComponent("memory", "memHierarchy.standardInterface")

c0_l1cache.addParams({
      "access_latency_cycles" : "3",
      "cache_frequency" : "2GHz",
      "replacement_policy" : "mru",
      "coherence_protocol" : "MESI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : DEBUG_L1,
      "debug_level" : DEBUG_LEVEL
})
cpu1 = sst.Component("core1", "Carcosa.FaultInjCPU")
cpu1.addParams({
    "clock" : "2.2GHz",
    "memFreq" : "4",
    "rngseed" : "301",
    "memSize" : "1MiB",
    "verbose" : 0,
    "maxOutstanding" : 16,
    "opCount" : 5000,
    "reqsPerIssue" : 4,
    "write_freq" : 40,
    "read_freq" : 60,
})
cpu1.addLink(halitoCPULink1, "haliToCPU", "10ns")
iface1 = cpu1.setSubComponent("memory", "memHierarchy.standardInterface")
c1_l1cache = sst.Component("l1cache1.mesi", "memHierarchy.Cache")
c1_l1cache.addParams({
      "access_latency_cycles" : "3",
      "cache_frequency" : "2GHz",
      "replacement_policy" : "mru",
      "coherence_protocol" : "MESI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : DEBUG_L1,
      "debug_level" : DEBUG_LEVEL
})
n0_bus = sst.Component("bus0", "memHierarchy.Bus")
n0_bus.addParams({
      "bus_frequency" : "2GHz"
})
n0_l2cache = sst.Component("l2cache0.mesi.inclus", "memHierarchy.Cache")
n0_l2cache.addParams({
      "access_latency_cycles" : "11",
      "cache_frequency" : "2GHz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "32 KB",
      "debug" : DEBUG_L2,
      "debug_level" : DEBUG_LEVEL
})
cpu2 = sst.Component("core2", "Carcosa.FaultInjCPU")
cpu2.addParams({
    "clock" : "2.2GHz",
    "memFreq" : "4",
    "rngseed" : "501",
    "memSize" : "1MiB",
    "verbose" : 0,
    "maxOutstanding" : 16,
    "opCount" : 5000,
    "reqsPerIssue" : 4,
    "write_freq" : 40,
    "read_freq" : 60,
})
cpu2.addLink(halitoCPULink2, "haliToCPU", "10ns")
iface2 = cpu2.setSubComponent("memory", "memHierarchy.standardInterface")
c2_l1cache = sst.Component("l1cache2.mesi", "memHierarchy.Cache")
c2_l1cache.addParams({
      "access_latency_cycles" : "3",
      "cache_frequency" : "2GHz",
      "replacement_policy" : "mru",
      "coherence_protocol" : "MESI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : DEBUG_L1,
      "debug_level" : DEBUG_LEVEL
})
cpu3 = sst.Component("core3", "Carcosa.FaultInjCPU")
cpu3.addParams({
    "clock" : "2.2GHz",
    "memFreq" : "4",
    "rngseed" : "701",
    "memSize" : "1MiB",
    "verbose" : 0,
    "maxOutstanding" : 16,
    "opCount" : 5000,
    "reqsPerIssue" : 4,
    "write_freq" : 40,
    "read_freq" : 60,
})
cpu3.addLink(halitoCPULink3, "haliToCPU", "10ns")
iface3 = cpu3.setSubComponent("memory", "memHierarchy.standardInterface")
c3_l1cache = sst.Component("l1cache3.mesi", "memHierarchy.Cache")
c3_l1cache.addParams({
      "access_latency_cycles" : "3",
      "cache_frequency" : "2GHz",
      "replacement_policy" : "mru",
      "coherence_protocol" : "MESI",
      "associativity" : "4",
      "cache_line_size" : "64",
      "cache_size" : "4 KB",
      "L1" : "1",
      "debug" : DEBUG_L1,
      "debug_level" : DEBUG_LEVEL
})
n1_bus = sst.Component("bus1", "memHierarchy.Bus")
n1_bus.addParams({
      "bus_frequency" : "2GHz"
})
n1_l2cache = sst.Component("l2cache1.mesi.inclus", "memHierarchy.Cache")
n1_l2cache.addParams({
      "access_latency_cycles" : "11",
      "cache_frequency" : "2GHz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "cache_size" : "32 KB",
      "debug" : DEBUG_L2,
      "debug_level" : DEBUG_LEVEL
})
n2_bus = sst.Component("bus2", "memHierarchy.Bus")
n2_bus.addParams({
      "bus_frequency" : "2GHz"
})
l3cache = sst.Component("l3cache.mesi.inclus", "memHierarchy.Cache")
l3cache.addParams({
      "access_latency_cycles" : "19",
      "cache_frequency" : "2GHz",
      "replacement_policy" : "nmru",
      "coherence_protocol" : "MESI",
      "associativity" : "16",
      "cache_line_size" : "64",
      "cache_size" : "64KiB",
      "debug" : DEBUG_L3,
      "debug_level" : DEBUG_LEVEL
})

l3NIC = l3cache.setSubComponent("lowlink", "memHierarchy.MemNIC")
l3NIC.addParams({
    "network_bw" : "40GB/s",
    "input_buffer_size" : "2KiB",
    "output_buffer_size" : "2KiB",
    "group" : 1,
})

network = sst.Component("network", "merlin.hr_router")
network.addParams({
      "xbar_bw" : "30GB/s",
      "link_bw" : "30GB/s",
      "input_buf_size" : "2KiB",
      "num_ports" : "2",
      "flit_size" : "36B",
      "output_buf_size" : "2KiB",
      "id" : "0",
      "topology" : "merlin.singlerouter"
})
network.setSubComponent("topology","merlin.singlerouter")
dirctrl = sst.Component("directory.mesi", "memHierarchy.DirectoryController")
dirctrl.addParams({
    "clock" : "1.5GHz",
    "coherence_protocol" : "MESI",
    "debug" : DEBUG_DIR,
    "debug_level" : DEBUG_LEVEL,
    "entry_cache_size" : "16384",
    "addr_range_end" : "0x1F000000",
    "addr_range_start" : "0x0",
})
dirNIC = dirctrl.setSubComponent("highlink", "memHierarchy.MemNIC")
dirNIC.addParams({
    "network_bw" : "40GB/s",
    "input_buffer_size" : "2KiB",
    "output_buffer_size" : "2KiB",
    "group" : 2,
})
memctrl = sst.Component("memory", "Carcosa.CarcosaMemCtrl")
memctrl.addParams({
    "clock" : "500MHz",
    "backing" : "hybrid",
    "addr_range_end" : 512*1024*1024-1,
    "debug" : DEBUG_MEM,
    "debug_level" : DEBUG_LEVEL,
    "numHaliLinks" : 4
})

memctrl.addLink(haliCtrl0, "haliLinks_0", "10ns")
memctrl.addLink(haliCtrl1, "haliLinks_1", "10ns")
memctrl.addLink(haliCtrl2, "haliLinks_2", "10ns")
memctrl.addLink(haliCtrl3, "haliLinks_3", "10ns")
memdelay = memctrl.setSubComponent("backend", "memHierarchy.DelayBuffer")
memreorder = memdelay.setSubComponent("backend", "memHierarchy.reorderByRow")
memory = memreorder.setSubComponent("backend", "memHierarchy.simpleDRAM")
memdelay.addParams({
    "max_requests_per_cycle" : 50,
    "request_delay" : "20ns",
})
memreorder.addParams({
    "max_issue_per_cycle" : 2,
    "reorder_limit" : "20",
})
memory.addParams({
    "mem_size" : "512MiB",
    "tCAS" : 3,
    "tRCD" : 3,
    "tRP" : 3,
    "cycle_time" : "5ns",
    "row_size" : "8KiB",
    "row_policy" : "open"
})

# Enable statistics
sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputConsole")
for a in componentlist:
    sst.enableAllStatisticsForComponentType(a)

# Links: CPU <-> Hali <-> L1
link_c0_hali = sst.Link("link_c0_hali")
link_hali0_l1cache = sst.Link("link_hali0_l1cache")
link_c0_hali.connect( (iface0, "lowlink", "100ps"), (hali_0, "highlink", "100ps") )
link_hali0_l1cache.connect( (hali_0, "lowlink", "100ps"), (c0_l1cache, "highlink", "100ps") )

link_c1_hali = sst.Link("link_c1_hali")
link_hali1_l1cache = sst.Link("link_hali1_l1cache")
link_c1_hali.connect( (iface1, "lowlink", "100ps"), (hali_1, "highlink", "100ps") )
link_hali1_l1cache.connect( (hali_1, "lowlink", "100ps"), (c1_l1cache, "highlink", "100ps") )

link_c2_hali = sst.Link("link_c2_hali")
link_hali2_l1cache = sst.Link("link_hali2_l1cache")
link_c2_hali.connect( (iface2, "lowlink", "100ps"), (hali_2, "highlink", "100ps") )
link_hali2_l1cache.connect( (hali_2, "lowlink", "100ps"), (c2_l1cache, "highlink", "100ps") )

link_c3_hali = sst.Link("link_c3_hali")
link_hali3_l1cache = sst.Link("link_hali3_l1cache")
link_c3_hali.connect( (iface3, "lowlink", "100ps"), (hali_3, "highlink", "100ps") )
link_hali3_l1cache.connect( (hali_3, "lowlink", "100ps"), (c3_l1cache, "highlink", "100ps") )

link_c0L1cache_bus = sst.Link("link_c0L1cache_bus")
link_c0L1cache_bus.connect( (c0_l1cache, "lowlink", "200ps"), (n0_bus, "highlink0", "200ps") )

link_c1L1cache_bus = sst.Link("link_c1L1cache_bus")
link_c1L1cache_bus.connect( (c1_l1cache, "lowlink", "100ps"), (n0_bus, "highlink1", "200ps") )
link_bus_n0L2cache = sst.Link("link_bus_n0L2cache")
link_bus_n0L2cache.connect( (n0_bus, "lowlink0", "200ps"), (n0_l2cache, "highlink", "200ps") )
link_n0L2cache_bus = sst.Link("link_n0L2cache_bus")
link_n0L2cache_bus.connect( (n0_l2cache, "lowlink", "200ps"), (n2_bus, "highlink0", "200ps") )
link_c2L1cache_bus = sst.Link("link_c2L1cache_bus")
link_c2L1cache_bus.connect( (c2_l1cache, "lowlink", "200ps"), (n1_bus, "highlink0", "200ps") )
link_c3L1cache_bus = sst.Link("link_c3L1cache_bus")
link_c3L1cache_bus.connect( (c3_l1cache, "lowlink", "200ps"), (n1_bus, "highlink1", "200ps") )
link_bus_n1L2cache = sst.Link("link_bus_n1L2cache")
link_bus_n1L2cache.connect( (n1_bus, "lowlink0", "200ps"), (n1_l2cache, "highlink", "200ps") )
link_n1L2cache_bus = sst.Link("link_n1L2cache_bus")
link_n1L2cache_bus.connect( (n1_l2cache, "lowlink", "200ps"), (n2_bus, "highlink1", "200ps") )
link_bus_l3cache = sst.Link("link_bus_l3cache")
link_bus_l3cache.connect( (n2_bus, "lowlink0", "200ps"), (l3cache, "highlink", "200ps") )
link_cache_net_0 = sst.Link("link_cache_net_0")
link_cache_net_0.connect( (l3NIC, "port", "200ps"), (network, "port1", "150ps") )
link_dir_net_0 = sst.Link("link_dir_net_0")

# Fault injector on each L1: each registers with exactly one registry so that Hali N manages only L1 N.
# debugManagerLogic=True enables [ManagerLogic] and PM-read debug prints.
c0_l1cache.addPortModule("lowlink", "carcosa.faultInjectorMemH", {
        "pmId": "l1_0",
        "pmRegistryIds": "hali_0",
        "faultType": "randomFlip",
        "injectionProbability": 0.5,
        "installDirection": "Send",
        "debugManagerLogic": True
})
c1_l1cache.addPortModule("lowlink", "carcosa.faultInjectorMemH", {
        "pmId": "l1_1",
        "pmRegistryIds": "hali_1",
        "faultType": "randomFlip",
        "injectionProbability": 0.5,
        "installDirection": "Send",
        "debugManagerLogic": True
})
c2_l1cache.addPortModule("lowlink", "carcosa.faultInjectorMemH", {
        "pmId": "l1_2",
        "pmRegistryIds": "hali_2",
        "faultType": "randomFlip",
        "injectionProbability": 0.5,
        "installDirection": "Send",
        "debugManagerLogic": True
})
c3_l1cache.addPortModule("lowlink", "carcosa.faultInjectorMemH", {
        "pmId": "l1_3",
        "pmRegistryIds": "hali_3",
        "faultType": "randomFlip",
        "injectionProbability": 0.5,
        "installDirection": "Send",
        "debugManagerLogic": True
})

link_dir_net_0.connect( (network, "port0", "150ps"), (dirNIC, "port", "150ps") )
link_dir_mem_link = sst.Link("link_dir_mem_link")
link_dir_mem_link.connect( (dirctrl, "lowlink", "200ps"), (memctrl, "highlink", "200ps") )
