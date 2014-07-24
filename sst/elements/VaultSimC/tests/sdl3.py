# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "0 ns")

# Define the simulation components
comp_cores = sst.Component("cores", "m5C.M5")
comp_cores.addParams({
      "info" : """yes""",
      "M5debugFile" : """m5.log""",
      "M5debug" : """""",
      "registerExit" : """yes""",
      "configFile" : """stream-8coreVault_1.6GHz-M5.xml""",
      "frequency" : """2.9 GHz""",
      "statFile" : """stream-8coreVault_1.6GHz-M5.stats""",
      "mem_initializer_port" : """n0.core0_L1D"""
})
comp_dirctrl0 = sst.Component("dirctrl0", "memHierarchy.DirectoryController")
comp_dirctrl0.addParams({
      "addrRangeStart" : """0x0""",
      "addrRangeEnd" : """0x1FE000000""",
      "network_bw" : """25.6GHz""",
      "interleaveStep" : """8""",
      "network_address" : """0""",
      "backingStoreSize" : """0x1000000""",
      "interleaveSize" : """4""",
      "printStats" : """""",
      "debug" : """""",
      "entryCacheSize" : """16384"""
})
comp_dirctrl1 = sst.Component("dirctrl1", "memHierarchy.DirectoryController")
comp_dirctrl1.addParams({
      "addrRangeStart" : """0x1000""",
      "addrRangeEnd" : """0x1FE000000""",
      "network_bw" : """25.6GHz""",
      "interleaveStep" : """8""",
      "network_address" : """1""",
      "backingStoreSize" : """0x1000000""",
      "interleaveSize" : """4""",
      "printStats" : """""",
      "debug" : """""",
      "entryCacheSize" : """16384"""
})
comp_memory0 = sst.Component("memory0", "memHierarchy.MemController")
comp_memory0.addParams({
      "printStats" : """1""",
      "mem_size" : """1024""",
      "clock" : """1GHz""",
      "backend" : """memHierarchy.vaultsim"""
})
comp_LL0 = sst.Component("LL0", "VaultSimC.logicLayer")
comp_LL0.addParams({
      "clock" : """500Mhz""",
      "vaults" : """16""",
      "terminal" : """1""",
      "llID" : """0""",
      "LL_MASK" : """0""",
      "bwlimit" : """8"""
})
comp_Vault0_0 = sst.Component("Vault0.0", "VaultSimC.VaultSimC")
comp_Vault0_0.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """0""",
      "numVaults2" : """4"""
})
comp_Vault0_1 = sst.Component("Vault0.1", "VaultSimC.VaultSimC")
comp_Vault0_1.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """1""",
      "numVaults2" : """4"""
})
comp_Vault0_2 = sst.Component("Vault0.2", "VaultSimC.VaultSimC")
comp_Vault0_2.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """2""",
      "numVaults2" : """4"""
})
comp_Vault0_3 = sst.Component("Vault0.3", "VaultSimC.VaultSimC")
comp_Vault0_3.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """3""",
      "numVaults2" : """4"""
})
comp_Vault0_4 = sst.Component("Vault0.4", "VaultSimC.VaultSimC")
comp_Vault0_4.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """4""",
      "numVaults2" : """4"""
})
comp_Vault0_5 = sst.Component("Vault0.5", "VaultSimC.VaultSimC")
comp_Vault0_5.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """5""",
      "numVaults2" : """4"""
})
comp_Vault0_6 = sst.Component("Vault0.6", "VaultSimC.VaultSimC")
comp_Vault0_6.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """6""",
      "numVaults2" : """4"""
})
comp_Vault0_7 = sst.Component("Vault0.7", "VaultSimC.VaultSimC")
comp_Vault0_7.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """7""",
      "numVaults2" : """4"""
})
comp_Vault0_8 = sst.Component("Vault0.8", "VaultSimC.VaultSimC")
comp_Vault0_8.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """8""",
      "numVaults2" : """4"""
})
comp_Vault0_9 = sst.Component("Vault0.9", "VaultSimC.VaultSimC")
comp_Vault0_9.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """9""",
      "numVaults2" : """4"""
})
comp_Vault0_10 = sst.Component("Vault0.10", "VaultSimC.VaultSimC")
comp_Vault0_10.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """10""",
      "numVaults2" : """4"""
})
comp_Vault0_11 = sst.Component("Vault0.11", "VaultSimC.VaultSimC")
comp_Vault0_11.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """11""",
      "numVaults2" : """4"""
})
comp_Vault0_12 = sst.Component("Vault0.12", "VaultSimC.VaultSimC")
comp_Vault0_12.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """12""",
      "numVaults2" : """4"""
})
comp_Vault0_13 = sst.Component("Vault0.13", "VaultSimC.VaultSimC")
comp_Vault0_13.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """13""",
      "numVaults2" : """4"""
})
comp_Vault0_14 = sst.Component("Vault0.14", "VaultSimC.VaultSimC")
comp_Vault0_14.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """14""",
      "numVaults2" : """4"""
})
comp_Vault0_15 = sst.Component("Vault0.15", "VaultSimC.VaultSimC")
comp_Vault0_15.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """15""",
      "numVaults2" : """4"""
})
comp_memory1 = sst.Component("memory1", "memHierarchy.MemController")
comp_memory1.addParams({
      "printStats" : """1""",
      "mem_size" : """1024""",
      "clock" : """1GHz""",
      "backend" : """memHierarchy.vaultsim"""
})
comp_LL1 = sst.Component("LL1", "VaultSimC.logicLayer")
comp_LL1.addParams({
      "clock" : """500Mhz""",
      "vaults" : """16""",
      "terminal" : """1""",
      "llID" : """0""",
      "LL_MASK" : """0""",
      "bwlimit" : """8"""
})
comp_Vault1_0 = sst.Component("Vault1.0", "VaultSimC.VaultSimC")
comp_Vault1_0.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """0""",
      "numVaults2" : """4"""
})
comp_Vault1_1 = sst.Component("Vault1.1", "VaultSimC.VaultSimC")
comp_Vault1_1.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """1""",
      "numVaults2" : """4"""
})
comp_Vault1_2 = sst.Component("Vault1.2", "VaultSimC.VaultSimC")
comp_Vault1_2.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """2""",
      "numVaults2" : """4"""
})
comp_Vault1_3 = sst.Component("Vault1.3", "VaultSimC.VaultSimC")
comp_Vault1_3.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """3""",
      "numVaults2" : """4"""
})
comp_Vault1_4 = sst.Component("Vault1.4", "VaultSimC.VaultSimC")
comp_Vault1_4.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """4""",
      "numVaults2" : """4"""
})
comp_Vault1_5 = sst.Component("Vault1.5", "VaultSimC.VaultSimC")
comp_Vault1_5.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """5""",
      "numVaults2" : """4"""
})
comp_Vault1_6 = sst.Component("Vault1.6", "VaultSimC.VaultSimC")
comp_Vault1_6.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """6""",
      "numVaults2" : """4"""
})
comp_Vault1_7 = sst.Component("Vault1.7", "VaultSimC.VaultSimC")
comp_Vault1_7.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """7""",
      "numVaults2" : """4"""
})
comp_Vault1_8 = sst.Component("Vault1.8", "VaultSimC.VaultSimC")
comp_Vault1_8.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """8""",
      "numVaults2" : """4"""
})
comp_Vault1_9 = sst.Component("Vault1.9", "VaultSimC.VaultSimC")
comp_Vault1_9.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """9""",
      "numVaults2" : """4"""
})
comp_Vault1_10 = sst.Component("Vault1.10", "VaultSimC.VaultSimC")
comp_Vault1_10.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """10""",
      "numVaults2" : """4"""
})
comp_Vault1_11 = sst.Component("Vault1.11", "VaultSimC.VaultSimC")
comp_Vault1_11.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """11""",
      "numVaults2" : """4"""
})
comp_Vault1_12 = sst.Component("Vault1.12", "VaultSimC.VaultSimC")
comp_Vault1_12.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """12""",
      "numVaults2" : """4"""
})
comp_Vault1_13 = sst.Component("Vault1.13", "VaultSimC.VaultSimC")
comp_Vault1_13.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """13""",
      "numVaults2" : """4"""
})
comp_Vault1_14 = sst.Component("Vault1.14", "VaultSimC.VaultSimC")
comp_Vault1_14.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """14""",
      "numVaults2" : """4"""
})
comp_Vault1_15 = sst.Component("Vault1.15", "VaultSimC.VaultSimC")
comp_Vault1_15.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """15""",
      "numVaults2" : """4"""
})
comp_n0_core0_l1Dcache = sst.Component("n0.core0:l1Dcache", "memHierarchy.Cache")
comp_n0_core0_l1Dcache.addParams({
      "printStats" : """""",
      "debug" : """""",
      "next_level" : """n0.l2cache""",
      "num_upstream" : """1""",
      "num_ways" : """2""",
      "blocksize" : """64""",
      "prefetcher" : """cassini.StridePrefetcher""",
      "num_rows" : """32""",
      "access_time" : """1336 ps""",
      "maxL1ResponseTime" : """70000"""
})
comp_n0_core0_l1Icache = sst.Component("n0.core0:l1Icache", "memHierarchy.Cache")
comp_n0_core0_l1Icache.addParams({
      "printStats" : """""",
      "debug" : """""",
      "next_level" : """n0.l2cache""",
      "num_upstream" : """1""",
      "num_ways" : """8""",
      "blocksize" : """64""",
      "num_rows" : """64""",
      "access_time" : """1336 ps""",
      "maxL1ResponseTime" : """70000"""
})
comp_n0_core1_l1Dcache = sst.Component("n0.core1:l1Dcache", "memHierarchy.Cache")
comp_n0_core1_l1Dcache.addParams({
      "printStats" : """""",
      "debug" : """""",
      "next_level" : """n0.l2cache""",
      "num_upstream" : """1""",
      "num_ways" : """2""",
      "blocksize" : """64""",
      "prefetcher" : """cassini.StridePrefetcher""",
      "num_rows" : """32""",
      "access_time" : """1336 ps""",
      "maxL1ResponseTime" : """70000"""
})
comp_n0_core1_l1Icache = sst.Component("n0.core1:l1Icache", "memHierarchy.Cache")
comp_n0_core1_l1Icache.addParams({
      "printStats" : """""",
      "debug" : """""",
      "next_level" : """n0.l2cache""",
      "num_upstream" : """1""",
      "num_ways" : """8""",
      "blocksize" : """64""",
      "num_rows" : """64""",
      "access_time" : """1336 ps""",
      "maxL1ResponseTime" : """70000"""
})
comp_n0_core2_l1Dcache = sst.Component("n0.core2:l1Dcache", "memHierarchy.Cache")
comp_n0_core2_l1Dcache.addParams({
      "printStats" : """""",
      "debug" : """""",
      "next_level" : """n0.l2cache""",
      "num_upstream" : """1""",
      "num_ways" : """2""",
      "blocksize" : """64""",
      "prefetcher" : """cassini.StridePrefetcher""",
      "num_rows" : """32""",
      "access_time" : """1336 ps""",
      "maxL1ResponseTime" : """70000"""
})
comp_n0_core2_l1Icache = sst.Component("n0.core2:l1Icache", "memHierarchy.Cache")
comp_n0_core2_l1Icache.addParams({
      "printStats" : """""",
      "debug" : """""",
      "next_level" : """n0.l2cache""",
      "num_upstream" : """1""",
      "num_ways" : """8""",
      "blocksize" : """64""",
      "num_rows" : """64""",
      "access_time" : """1336 ps""",
      "maxL1ResponseTime" : """70000"""
})
comp_n0_core3_l1Dcache = sst.Component("n0.core3:l1Dcache", "memHierarchy.Cache")
comp_n0_core3_l1Dcache.addParams({
      "printStats" : """""",
      "debug" : """""",
      "next_level" : """n0.l2cache""",
      "num_upstream" : """1""",
      "num_ways" : """2""",
      "blocksize" : """64""",
      "prefetcher" : """cassini.StridePrefetcher""",
      "num_rows" : """32""",
      "access_time" : """1336 ps""",
      "maxL1ResponseTime" : """70000"""
})
comp_n0_core3_l1Icache = sst.Component("n0.core3:l1Icache", "memHierarchy.Cache")
comp_n0_core3_l1Icache.addParams({
      "printStats" : """""",
      "debug" : """""",
      "next_level" : """n0.l2cache""",
      "num_upstream" : """1""",
      "num_ways" : """8""",
      "blocksize" : """64""",
      "num_rows" : """64""",
      "access_time" : """1336 ps""",
      "maxL1ResponseTime" : """70000"""
})
comp_n0_l2cache = sst.Component("n0.l2cache", "memHierarchy.Cache")
comp_n0_l2cache.addParams({
      "printStats" : """""",
      "debug" : """""",
      "num_ways" : """8""",
      "blocksize" : """64""",
      "prefetcher" : """cassini.NextBlockPrefetcher""",
      "num_rows" : """256""",
      "access_time" : """4880 ps""",
      "net_addr" : """2""",
      "mode" : """INCLUSIVE"""
})
comp_n0_membus = sst.Component("n0.membus", "memHierarchy.Bus")
comp_n0_membus.addParams({
      "debug" : """""",
      "numPorts" : """9""",
      "busDelay" : """1ns"""
})
comp_n1_core0_l1Dcache = sst.Component("n1.core0:l1Dcache", "memHierarchy.Cache")
comp_n1_core0_l1Dcache.addParams({
      "printStats" : """""",
      "debug" : """""",
      "next_level" : """n1.l2cache""",
      "num_upstream" : """1""",
      "num_ways" : """2""",
      "blocksize" : """64""",
      "prefetcher" : """cassini.StridePrefetcher""",
      "num_rows" : """32""",
      "access_time" : """1336 ps""",
      "maxL1ResponseTime" : """70000"""
})
comp_n1_core0_l1Icache = sst.Component("n1.core0:l1Icache", "memHierarchy.Cache")
comp_n1_core0_l1Icache.addParams({
      "printStats" : """""",
      "debug" : """""",
      "next_level" : """n1.l2cache""",
      "num_upstream" : """1""",
      "num_ways" : """8""",
      "blocksize" : """64""",
      "num_rows" : """64""",
      "access_time" : """1336 ps""",
      "maxL1ResponseTime" : """70000"""
})
comp_n1_core1_l1Dcache = sst.Component("n1.core1:l1Dcache", "memHierarchy.Cache")
comp_n1_core1_l1Dcache.addParams({
      "printStats" : """""",
      "debug" : """""",
      "next_level" : """n1.l2cache""",
      "num_upstream" : """1""",
      "num_ways" : """2""",
      "blocksize" : """64""",
      "prefetcher" : """cassini.StridePrefetcher""",
      "num_rows" : """32""",
      "access_time" : """1336 ps""",
      "maxL1ResponseTime" : """70000"""
})
comp_n1_core1_l1Icache = sst.Component("n1.core1:l1Icache", "memHierarchy.Cache")
comp_n1_core1_l1Icache.addParams({
      "printStats" : """""",
      "debug" : """""",
      "next_level" : """n1.l2cache""",
      "num_upstream" : """1""",
      "num_ways" : """8""",
      "blocksize" : """64""",
      "num_rows" : """64""",
      "access_time" : """1336 ps""",
      "maxL1ResponseTime" : """70000"""
})
comp_n1_core2_l1Dcache = sst.Component("n1.core2:l1Dcache", "memHierarchy.Cache")
comp_n1_core2_l1Dcache.addParams({
      "printStats" : """""",
      "debug" : """""",
      "next_level" : """n1.l2cache""",
      "num_upstream" : """1""",
      "num_ways" : """2""",
      "blocksize" : """64""",
      "prefetcher" : """cassini.StridePrefetcher""",
      "num_rows" : """32""",
      "access_time" : """1336 ps""",
      "maxL1ResponseTime" : """70000"""
})
comp_n1_core2_l1Icache = sst.Component("n1.core2:l1Icache", "memHierarchy.Cache")
comp_n1_core2_l1Icache.addParams({
      "printStats" : """""",
      "debug" : """""",
      "next_level" : """n1.l2cache""",
      "num_upstream" : """1""",
      "num_ways" : """8""",
      "blocksize" : """64""",
      "num_rows" : """64""",
      "access_time" : """1336 ps""",
      "maxL1ResponseTime" : """70000"""
})
comp_n1_core3_l1Dcache = sst.Component("n1.core3:l1Dcache", "memHierarchy.Cache")
comp_n1_core3_l1Dcache.addParams({
      "printStats" : """""",
      "debug" : """""",
      "next_level" : """n1.l2cache""",
      "num_upstream" : """1""",
      "num_ways" : """2""",
      "blocksize" : """64""",
      "prefetcher" : """cassini.StridePrefetcher""",
      "num_rows" : """32""",
      "access_time" : """1336 ps""",
      "maxL1ResponseTime" : """70000"""
})
comp_n1_core3_l1Icache = sst.Component("n1.core3:l1Icache", "memHierarchy.Cache")
comp_n1_core3_l1Icache.addParams({
      "printStats" : """""",
      "debug" : """""",
      "next_level" : """n1.l2cache""",
      "num_upstream" : """1""",
      "num_ways" : """8""",
      "blocksize" : """64""",
      "num_rows" : """64""",
      "access_time" : """1336 ps""",
      "maxL1ResponseTime" : """70000"""
})
comp_n1_l2cache = sst.Component("n1.l2cache", "memHierarchy.Cache")
comp_n1_l2cache.addParams({
      "printStats" : """""",
      "debug" : """""",
      "num_ways" : """8""",
      "blocksize" : """64""",
      "prefetcher" : """cassini.NextBlockPrefetcher""",
      "num_rows" : """256""",
      "access_time" : """4880 ps""",
      "net_addr" : """3""",
      "mode" : """INCLUSIVE"""
})
comp_n1_membus = sst.Component("n1.membus", "memHierarchy.Bus")
comp_n1_membus.addParams({
      "debug" : """""",
      "numPorts" : """9""",
      "busDelay" : """1ns"""
})
comp_chipRtr = sst.Component("chipRtr", "merlin.hr_router")
comp_chipRtr.addParams({
      "num_vcs" : """3""",
      "xbar_bw" : """25.6GHz""",
      "link_bw" : """25.6GHz""",
      "num_ports" : """4""",
      "id" : """0""",
      "topology" : """merlin.singlerouter"""
})


# Define the simulation links
link_n0_core0_L1D = sst.Link("link_n0_core0_L1D")
link_n0_core0_L1D.connect( (comp_cores, "n0.core0_L1D", "1000ps"), (comp_n0_core0_l1Dcache, "upstream0", "1000ps") )
link_n0_core0_L1I = sst.Link("link_n0_core0_L1I")
link_n0_core0_L1I.connect( (comp_cores, "n0.core0_L1I", "1000ps"), (comp_n0_core0_l1Icache, "upstream0", "1000ps") )
link_n0_core1_L1D = sst.Link("link_n0_core1_L1D")
link_n0_core1_L1D.connect( (comp_cores, "n0.core1_L1D", "1000ps"), (comp_n0_core1_l1Dcache, "upstream0", "1000ps") )
link_n0_core1_L1I = sst.Link("link_n0_core1_L1I")
link_n0_core1_L1I.connect( (comp_cores, "n0.core1_L1I", "1000ps"), (comp_n0_core1_l1Icache, "upstream0", "1000ps") )
link_n0_core2_L1D = sst.Link("link_n0_core2_L1D")
link_n0_core2_L1D.connect( (comp_cores, "n0.core2_L1D", "1000ps"), (comp_n0_core2_l1Dcache, "upstream0", "1000ps") )
link_n0_core2_L1I = sst.Link("link_n0_core2_L1I")
link_n0_core2_L1I.connect( (comp_cores, "n0.core2_L1I", "1000ps"), (comp_n0_core2_l1Icache, "upstream0", "1000ps") )
link_n0_core3_L1D = sst.Link("link_n0_core3_L1D")
link_n0_core3_L1D.connect( (comp_cores, "n0.core3_L1D", "1000ps"), (comp_n0_core3_l1Dcache, "upstream0", "1000ps") )
link_n0_core3_L1I = sst.Link("link_n0_core3_L1I")
link_n0_core3_L1I.connect( (comp_cores, "n0.core3_L1I", "1000ps"), (comp_n0_core3_l1Icache, "upstream0", "1000ps") )
link_n1_core0_L1D = sst.Link("link_n1_core0_L1D")
link_n1_core0_L1D.connect( (comp_cores, "n1.core0_L1D", "1000ps"), (comp_n1_core0_l1Dcache, "upstream0", "1000ps") )
link_n1_core0_L1I = sst.Link("link_n1_core0_L1I")
link_n1_core0_L1I.connect( (comp_cores, "n1.core0_L1I", "1000ps"), (comp_n1_core0_l1Icache, "upstream0", "1000ps") )
link_n1_core1_L1D = sst.Link("link_n1_core1_L1D")
link_n1_core1_L1D.connect( (comp_cores, "n1.core1_L1D", "1000ps"), (comp_n1_core1_l1Dcache, "upstream0", "1000ps") )
link_n1_core1_L1I = sst.Link("link_n1_core1_L1I")
link_n1_core1_L1I.connect( (comp_cores, "n1.core1_L1I", "1000ps"), (comp_n1_core1_l1Icache, "upstream0", "1000ps") )
link_n1_core2_L1D = sst.Link("link_n1_core2_L1D")
link_n1_core2_L1D.connect( (comp_cores, "n1.core2_L1D", "1000ps"), (comp_n1_core2_l1Dcache, "upstream0", "1000ps") )
link_n1_core2_L1I = sst.Link("link_n1_core2_L1I")
link_n1_core2_L1I.connect( (comp_cores, "n1.core2_L1I", "1000ps"), (comp_n1_core2_l1Icache, "upstream0", "1000ps") )
link_n1_core3_L1D = sst.Link("link_n1_core3_L1D")
link_n1_core3_L1D.connect( (comp_cores, "n1.core3_L1D", "1000ps"), (comp_n1_core3_l1Dcache, "upstream0", "1000ps") )
link_n1_core3_L1I = sst.Link("link_n1_core3_L1I")
link_n1_core3_L1I.connect( (comp_cores, "n1.core3_L1I", "1000ps"), (comp_n1_core3_l1Icache, "upstream0", "1000ps") )
link_dirctrl0_mem = sst.Link("link_dirctrl0_mem")
link_dirctrl0_mem.connect( (comp_dirctrl0, "memory", "50ps"), (comp_memory0, "direct_link", "50ps") )
link_dirctrl0_net = sst.Link("link_dirctrl0_net")
link_dirctrl0_net.connect( (comp_dirctrl0, "network", "50ps"), (comp_chipRtr, "port0", "10000ps") )
link_dirctrl1_mem = sst.Link("link_dirctrl1_mem")
link_dirctrl1_mem.connect( (comp_dirctrl1, "memory", "50ps"), (comp_memory1, "direct_link", "50ps") )
link_dirctrl1_net = sst.Link("link_dirctrl1_net")
link_dirctrl1_net.connect( (comp_dirctrl1, "network", "50ps"), (comp_chipRtr, "port1", "10000ps") )
link_chain0_c_0 = sst.Link("link_chain0_c_0")
link_chain0_c_0.connect( (comp_memory0, "cube_link", "6000ps"), (comp_LL0, "toCPU", "6000ps") )
link_LL0_0_0 = sst.Link("link_LL0_0_0")
link_LL0_0_0.connect( (comp_LL0, "bus_0", "3000ps"), (comp_Vault0_0, "bus", "3000ps") )
link_LL0_0_1 = sst.Link("link_LL0_0_1")
link_LL0_0_1.connect( (comp_LL0, "bus_1", "3000ps"), (comp_Vault0_1, "bus", "3000ps") )
link_LL0_0_2 = sst.Link("link_LL0_0_2")
link_LL0_0_2.connect( (comp_LL0, "bus_2", "3000ps"), (comp_Vault0_2, "bus", "3000ps") )
link_LL0_0_3 = sst.Link("link_LL0_0_3")
link_LL0_0_3.connect( (comp_LL0, "bus_3", "3000ps"), (comp_Vault0_3, "bus", "3000ps") )
link_LL0_0_4 = sst.Link("link_LL0_0_4")
link_LL0_0_4.connect( (comp_LL0, "bus_4", "3000ps"), (comp_Vault0_4, "bus", "3000ps") )
link_LL0_0_5 = sst.Link("link_LL0_0_5")
link_LL0_0_5.connect( (comp_LL0, "bus_5", "3000ps"), (comp_Vault0_5, "bus", "3000ps") )
link_LL0_0_6 = sst.Link("link_LL0_0_6")
link_LL0_0_6.connect( (comp_LL0, "bus_6", "3000ps"), (comp_Vault0_6, "bus", "3000ps") )
link_LL0_0_7 = sst.Link("link_LL0_0_7")
link_LL0_0_7.connect( (comp_LL0, "bus_7", "3000ps"), (comp_Vault0_7, "bus", "3000ps") )
link_LL0_0_8 = sst.Link("link_LL0_0_8")
link_LL0_0_8.connect( (comp_LL0, "bus_8", "3000ps"), (comp_Vault0_8, "bus", "3000ps") )
link_LL0_0_9 = sst.Link("link_LL0_0_9")
link_LL0_0_9.connect( (comp_LL0, "bus_9", "3000ps"), (comp_Vault0_9, "bus", "3000ps") )
link_LL0_0_10 = sst.Link("link_LL0_0_10")
link_LL0_0_10.connect( (comp_LL0, "bus_10", "3000ps"), (comp_Vault0_10, "bus", "3000ps") )
link_LL0_0_11 = sst.Link("link_LL0_0_11")
link_LL0_0_11.connect( (comp_LL0, "bus_11", "3000ps"), (comp_Vault0_11, "bus", "3000ps") )
link_LL0_0_12 = sst.Link("link_LL0_0_12")
link_LL0_0_12.connect( (comp_LL0, "bus_12", "3000ps"), (comp_Vault0_12, "bus", "3000ps") )
link_LL0_0_13 = sst.Link("link_LL0_0_13")
link_LL0_0_13.connect( (comp_LL0, "bus_13", "3000ps"), (comp_Vault0_13, "bus", "3000ps") )
link_LL0_0_14 = sst.Link("link_LL0_0_14")
link_LL0_0_14.connect( (comp_LL0, "bus_14", "3000ps"), (comp_Vault0_14, "bus", "3000ps") )
link_LL0_0_15 = sst.Link("link_LL0_0_15")
link_LL0_0_15.connect( (comp_LL0, "bus_15", "3000ps"), (comp_Vault0_15, "bus", "3000ps") )
link_chain1_c_0 = sst.Link("link_chain1_c_0")
link_chain1_c_0.connect( (comp_memory1, "cube_link", "6000ps"), (comp_LL1, "toCPU", "6000ps") )
link_LL1_0_0 = sst.Link("link_LL1_0_0")
link_LL1_0_0.connect( (comp_LL1, "bus_0", "3000ps"), (comp_Vault1_0, "bus", "3000ps") )
link_LL1_0_1 = sst.Link("link_LL1_0_1")
link_LL1_0_1.connect( (comp_LL1, "bus_1", "3000ps"), (comp_Vault1_1, "bus", "3000ps") )
link_LL1_0_2 = sst.Link("link_LL1_0_2")
link_LL1_0_2.connect( (comp_LL1, "bus_2", "3000ps"), (comp_Vault1_2, "bus", "3000ps") )
link_LL1_0_3 = sst.Link("link_LL1_0_3")
link_LL1_0_3.connect( (comp_LL1, "bus_3", "3000ps"), (comp_Vault1_3, "bus", "3000ps") )
link_LL1_0_4 = sst.Link("link_LL1_0_4")
link_LL1_0_4.connect( (comp_LL1, "bus_4", "3000ps"), (comp_Vault1_4, "bus", "3000ps") )
link_LL1_0_5 = sst.Link("link_LL1_0_5")
link_LL1_0_5.connect( (comp_LL1, "bus_5", "3000ps"), (comp_Vault1_5, "bus", "3000ps") )
link_LL1_0_6 = sst.Link("link_LL1_0_6")
link_LL1_0_6.connect( (comp_LL1, "bus_6", "3000ps"), (comp_Vault1_6, "bus", "3000ps") )
link_LL1_0_7 = sst.Link("link_LL1_0_7")
link_LL1_0_7.connect( (comp_LL1, "bus_7", "3000ps"), (comp_Vault1_7, "bus", "3000ps") )
link_LL1_0_8 = sst.Link("link_LL1_0_8")
link_LL1_0_8.connect( (comp_LL1, "bus_8", "3000ps"), (comp_Vault1_8, "bus", "3000ps") )
link_LL1_0_9 = sst.Link("link_LL1_0_9")
link_LL1_0_9.connect( (comp_LL1, "bus_9", "3000ps"), (comp_Vault1_9, "bus", "3000ps") )
link_LL1_0_10 = sst.Link("link_LL1_0_10")
link_LL1_0_10.connect( (comp_LL1, "bus_10", "3000ps"), (comp_Vault1_10, "bus", "3000ps") )
link_LL1_0_11 = sst.Link("link_LL1_0_11")
link_LL1_0_11.connect( (comp_LL1, "bus_11", "3000ps"), (comp_Vault1_11, "bus", "3000ps") )
link_LL1_0_12 = sst.Link("link_LL1_0_12")
link_LL1_0_12.connect( (comp_LL1, "bus_12", "3000ps"), (comp_Vault1_12, "bus", "3000ps") )
link_LL1_0_13 = sst.Link("link_LL1_0_13")
link_LL1_0_13.connect( (comp_LL1, "bus_13", "3000ps"), (comp_Vault1_13, "bus", "3000ps") )
link_LL1_0_14 = sst.Link("link_LL1_0_14")
link_LL1_0_14.connect( (comp_LL1, "bus_14", "3000ps"), (comp_Vault1_14, "bus", "3000ps") )
link_LL1_0_15 = sst.Link("link_LL1_0_15")
link_LL1_0_15.connect( (comp_LL1, "bus_15", "3000ps"), (comp_Vault1_15, "bus", "3000ps") )
link_n0_core0_l1d_bus = sst.Link("link_n0_core0_l1d_bus")
link_n0_core0_l1d_bus.connect( (comp_n0_core0_l1Dcache, "snoop_link", "1000ps"), (comp_n0_membus, "port0", "50ps") )
link_n0_core0_l1i_bus = sst.Link("link_n0_core0_l1i_bus")
link_n0_core0_l1i_bus.connect( (comp_n0_core0_l1Icache, "snoop_link", "1000ps"), (comp_n0_membus, "port1", "50ps") )
link_n0_core1_l1d_bus = sst.Link("link_n0_core1_l1d_bus")
link_n0_core1_l1d_bus.connect( (comp_n0_core1_l1Dcache, "snoop_link", "1000ps"), (comp_n0_membus, "port2", "50ps") )
link_n0_core1_l1i_bus = sst.Link("link_n0_core1_l1i_bus")
link_n0_core1_l1i_bus.connect( (comp_n0_core1_l1Icache, "snoop_link", "1000ps"), (comp_n0_membus, "port3", "50ps") )
link_n0_core2_l1d_bus = sst.Link("link_n0_core2_l1d_bus")
link_n0_core2_l1d_bus.connect( (comp_n0_core2_l1Dcache, "snoop_link", "1000ps"), (comp_n0_membus, "port4", "50ps") )
link_n0_core2_l1i_bus = sst.Link("link_n0_core2_l1i_bus")
link_n0_core2_l1i_bus.connect( (comp_n0_core2_l1Icache, "snoop_link", "1000ps"), (comp_n0_membus, "port5", "50ps") )
link_n0_core3_l1d_bus = sst.Link("link_n0_core3_l1d_bus")
link_n0_core3_l1d_bus.connect( (comp_n0_core3_l1Dcache, "snoop_link", "1000ps"), (comp_n0_membus, "port6", "50ps") )
link_n0_core3_l1i_bus = sst.Link("link_n0_core3_l1i_bus")
link_n0_core3_l1i_bus.connect( (comp_n0_core3_l1Icache, "snoop_link", "1000ps"), (comp_n0_membus, "port7", "50ps") )
link_n0_l2cache_bus = sst.Link("link_n0_l2cache_bus")
link_n0_l2cache_bus.connect( (comp_n0_l2cache, "snoop_link", "50ps"), (comp_n0_membus, "port8", "50ps") )
link_n0_l2cache_net = sst.Link("link_n0_l2cache_net")
link_n0_l2cache_net.connect( (comp_n0_l2cache, "directory", "10000ps"), (comp_chipRtr, "port2", "10000ps") )
link_n1_core0_l1d_bus = sst.Link("link_n1_core0_l1d_bus")
link_n1_core0_l1d_bus.connect( (comp_n1_core0_l1Dcache, "snoop_link", "1000ps"), (comp_n1_membus, "port0", "50ps") )
link_n1_core0_l1i_bus = sst.Link("link_n1_core0_l1i_bus")
link_n1_core0_l1i_bus.connect( (comp_n1_core0_l1Icache, "snoop_link", "1000ps"), (comp_n1_membus, "port1", "50ps") )
link_n1_core1_l1d_bus = sst.Link("link_n1_core1_l1d_bus")
link_n1_core1_l1d_bus.connect( (comp_n1_core1_l1Dcache, "snoop_link", "1000ps"), (comp_n1_membus, "port2", "50ps") )
link_n1_core1_l1i_bus = sst.Link("link_n1_core1_l1i_bus")
link_n1_core1_l1i_bus.connect( (comp_n1_core1_l1Icache, "snoop_link", "1000ps"), (comp_n1_membus, "port3", "50ps") )
link_n1_core2_l1d_bus = sst.Link("link_n1_core2_l1d_bus")
link_n1_core2_l1d_bus.connect( (comp_n1_core2_l1Dcache, "snoop_link", "1000ps"), (comp_n1_membus, "port4", "50ps") )
link_n1_core2_l1i_bus = sst.Link("link_n1_core2_l1i_bus")
link_n1_core2_l1i_bus.connect( (comp_n1_core2_l1Icache, "snoop_link", "1000ps"), (comp_n1_membus, "port5", "50ps") )
link_n1_core3_l1d_bus = sst.Link("link_n1_core3_l1d_bus")
link_n1_core3_l1d_bus.connect( (comp_n1_core3_l1Dcache, "snoop_link", "1000ps"), (comp_n1_membus, "port6", "50ps") )
link_n1_core3_l1i_bus = sst.Link("link_n1_core3_l1i_bus")
link_n1_core3_l1i_bus.connect( (comp_n1_core3_l1Icache, "snoop_link", "1000ps"), (comp_n1_membus, "port7", "50ps") )
link_n1_l2cache_bus = sst.Link("link_n1_l2cache_bus")
link_n1_l2cache_bus.connect( (comp_n1_l2cache, "snoop_link", "50ps"), (comp_n1_membus, "port8", "50ps") )
link_n1_l2cache_net = sst.Link("link_n1_l2cache_net")
link_n1_l2cache_net.connect( (comp_n1_l2cache, "directory", "10000ps"), (comp_chipRtr, "port3", "10000ps") )
# End of generated output.
