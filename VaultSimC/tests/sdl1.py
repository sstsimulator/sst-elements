# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "50us")

#enable stat output
sst.setStatisticLoadLevel(7)   
sst.setStatisticOutput("sst.statOutputCSV", {"filepath" : "./TestOutput.csv",
                                             "separator" : ", "
                                             })         

# Define the simulation components
comp_cpu = sst.Component("cpu", "VaultSimC.cpu")
comp_cpu.addParams({
      "app" : """0""",
      "seed" : """100""",
      "threads" : """2""",
      "bwlimit" : """4""",
      "clock" : """1500Mhz"""
})
comp_ll0 = sst.Component("ll0", "VaultSimC.logicLayer")
comp_ll0.addParams({
      "bwlimit" : """32""",
      "clock" : """500Mhz""",
      "vaults" : """8""",
      "terminal" : """1""",
      "llID" : """0""",
      "LL_MASK" : """0"""
})
comp_ll0.enableStatistics([
        "BW_recv_from_CPU"], {
        "type":"sst.AccumulatorStatistic",
        "rate":"0 ns"})
comp_ll0.enableStatistics([
        "BW_send_to_CPU"], {
        "type":"sst.AccumulatorStatistic",
        "rate":"0 ns"})

comp_c0_0 = sst.Component("c0.0", "VaultSimC.VaultSimC")
comp_c0_0.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """0""",
      "numVaults2" : """3"""
})
comp_c0_0.enableStatistics([
        "Mem_Outstanding"], {
        "type":"sst.AccumulatorStatistic",
        "rate":"0 ns"})

comp_c0_1 = sst.Component("c0.1", "VaultSimC.VaultSimC")
comp_c0_1.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """1""",
      "numVaults2" : """3"""
})
comp_c0_1.enableStatistics([
        "Mem_Outstanding"], {
        "type":"sst.AccumulatorStatistic",
        "rate":"0 ns"})

comp_c0_2 = sst.Component("c0.2", "VaultSimC.VaultSimC")
comp_c0_2.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """2""",
      "numVaults2" : """3"""
})
comp_c0_3 = sst.Component("c0.3", "VaultSimC.VaultSimC")
comp_c0_3.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """3""",
      "numVaults2" : """3"""
})
comp_c0_4 = sst.Component("c0.4", "VaultSimC.VaultSimC")
comp_c0_4.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """4""",
      "numVaults2" : """3"""
})
comp_c0_5 = sst.Component("c0.5", "VaultSimC.VaultSimC")
comp_c0_5.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """5""",
      "numVaults2" : """3"""
})
comp_c0_6 = sst.Component("c0.6", "VaultSimC.VaultSimC")
comp_c0_6.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """6""",
      "numVaults2" : """3"""
})
comp_c0_7 = sst.Component("c0.7", "VaultSimC.VaultSimC")
comp_c0_7.addParams({
      "clock" : """750Mhz""",
      "VaultID" : """7""",
      "numVaults2" : """3"""
})


# Define the simulation links
link_chain_c_0 = sst.Link("link_chain_c_0")
link_chain_c_0.connect( (comp_cpu, "toMem", "5000ps"), (comp_ll0, "toCPU", "5000ps") )
link_ll2V_0_0 = sst.Link("link_ll2V_0_0")
link_ll2V_0_0.connect( (comp_ll0, "bus_0", "1000ps"), (comp_c0_0, "bus", "1000ps") )
link_ll2V_0_1 = sst.Link("link_ll2V_0_1")
link_ll2V_0_1.connect( (comp_ll0, "bus_1", "1000ps"), (comp_c0_1, "bus", "1000ps") )
link_ll2V_0_2 = sst.Link("link_ll2V_0_2")
link_ll2V_0_2.connect( (comp_ll0, "bus_2", "1000ps"), (comp_c0_2, "bus", "1000ps") )
link_ll2V_0_3 = sst.Link("link_ll2V_0_3")
link_ll2V_0_3.connect( (comp_ll0, "bus_3", "1000ps"), (comp_c0_3, "bus", "1000ps") )
link_ll2V_0_4 = sst.Link("link_ll2V_0_4")
link_ll2V_0_4.connect( (comp_ll0, "bus_4", "1000ps"), (comp_c0_4, "bus", "1000ps") )
link_ll2V_0_5 = sst.Link("link_ll2V_0_5")
link_ll2V_0_5.connect( (comp_ll0, "bus_5", "1000ps"), (comp_c0_5, "bus", "1000ps") )
link_ll2V_0_6 = sst.Link("link_ll2V_0_6")
link_ll2V_0_6.connect( (comp_ll0, "bus_6", "1000ps"), (comp_c0_6, "bus", "1000ps") )
link_ll2V_0_7 = sst.Link("link_ll2V_0_7")
link_ll2V_0_7.connect( (comp_ll0, "bus_7", "1000ps"), (comp_c0_7, "bus", "1000ps") )
# End of generated output.
