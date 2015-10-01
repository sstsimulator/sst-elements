# Automatically generated SST Python input
import sst

# Define SST core options
sst.setProgramOption("timebase", "1 ps")
sst.setProgramOption("stopAtCycle", "0 ns")

# Define the simulation components
s_0_cpu = sst.Component("0.cpu", "portals4_sm.trig_cpu")
s_0_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """0"""
})
s_0_nic = sst.Component("0.nic", "iris.trig_nic")
s_0_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """0"""
})
s_0_rtr = sst.Component("0.rtr", "iris.router")
s_0_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """0"""
})
s_1_cpu = sst.Component("1.cpu", "portals4_sm.trig_cpu")
s_1_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """1"""
})
s_1_nic = sst.Component("1.nic", "iris.trig_nic")
s_1_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """1"""
})
s_1_rtr = sst.Component("1.rtr", "iris.router")
s_1_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """1"""
})
s_2_cpu = sst.Component("2.cpu", "portals4_sm.trig_cpu")
s_2_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """2"""
})
s_2_nic = sst.Component("2.nic", "iris.trig_nic")
s_2_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """2"""
})
s_2_rtr = sst.Component("2.rtr", "iris.router")
s_2_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """2"""
})
s_3_cpu = sst.Component("3.cpu", "portals4_sm.trig_cpu")
s_3_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """3"""
})
s_3_nic = sst.Component("3.nic", "iris.trig_nic")
s_3_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """3"""
})
s_3_rtr = sst.Component("3.rtr", "iris.router")
s_3_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """3"""
})
s_4_cpu = sst.Component("4.cpu", "portals4_sm.trig_cpu")
s_4_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """4"""
})
s_4_nic = sst.Component("4.nic", "iris.trig_nic")
s_4_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """4"""
})
s_4_rtr = sst.Component("4.rtr", "iris.router")
s_4_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """4"""
})
s_5_cpu = sst.Component("5.cpu", "portals4_sm.trig_cpu")
s_5_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """5"""
})
s_5_nic = sst.Component("5.nic", "iris.trig_nic")
s_5_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """5"""
})
s_5_rtr = sst.Component("5.rtr", "iris.router")
s_5_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """5"""
})
s_6_cpu = sst.Component("6.cpu", "portals4_sm.trig_cpu")
s_6_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """6"""
})
s_6_nic = sst.Component("6.nic", "iris.trig_nic")
s_6_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """6"""
})
s_6_rtr = sst.Component("6.rtr", "iris.router")
s_6_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """6"""
})
s_7_cpu = sst.Component("7.cpu", "portals4_sm.trig_cpu")
s_7_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """7"""
})
s_7_nic = sst.Component("7.nic", "iris.trig_nic")
s_7_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """7"""
})
s_7_rtr = sst.Component("7.rtr", "iris.router")
s_7_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """7"""
})
s_8_cpu = sst.Component("8.cpu", "portals4_sm.trig_cpu")
s_8_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """8"""
})
s_8_nic = sst.Component("8.nic", "iris.trig_nic")
s_8_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """8"""
})
s_8_rtr = sst.Component("8.rtr", "iris.router")
s_8_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """8"""
})
s_9_cpu = sst.Component("9.cpu", "portals4_sm.trig_cpu")
s_9_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """9"""
})
s_9_nic = sst.Component("9.nic", "iris.trig_nic")
s_9_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """9"""
})
s_9_rtr = sst.Component("9.rtr", "iris.router")
s_9_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """9"""
})
s_10_cpu = sst.Component("10.cpu", "portals4_sm.trig_cpu")
s_10_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """10"""
})
s_10_nic = sst.Component("10.nic", "iris.trig_nic")
s_10_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """10"""
})
s_10_rtr = sst.Component("10.rtr", "iris.router")
s_10_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """10"""
})
s_11_cpu = sst.Component("11.cpu", "portals4_sm.trig_cpu")
s_11_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """11"""
})
s_11_nic = sst.Component("11.nic", "iris.trig_nic")
s_11_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """11"""
})
s_11_rtr = sst.Component("11.rtr", "iris.router")
s_11_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """11"""
})
s_12_cpu = sst.Component("12.cpu", "portals4_sm.trig_cpu")
s_12_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """12"""
})
s_12_nic = sst.Component("12.nic", "iris.trig_nic")
s_12_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """12"""
})
s_12_rtr = sst.Component("12.rtr", "iris.router")
s_12_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """12"""
})
s_13_cpu = sst.Component("13.cpu", "portals4_sm.trig_cpu")
s_13_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """13"""
})
s_13_nic = sst.Component("13.nic", "iris.trig_nic")
s_13_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """13"""
})
s_13_rtr = sst.Component("13.rtr", "iris.router")
s_13_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """13"""
})
s_14_cpu = sst.Component("14.cpu", "portals4_sm.trig_cpu")
s_14_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """14"""
})
s_14_nic = sst.Component("14.nic", "iris.trig_nic")
s_14_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """14"""
})
s_14_rtr = sst.Component("14.rtr", "iris.router")
s_14_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """14"""
})
s_15_cpu = sst.Component("15.cpu", "portals4_sm.trig_cpu")
s_15_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """15"""
})
s_15_nic = sst.Component("15.nic", "iris.trig_nic")
s_15_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """15"""
})
s_15_rtr = sst.Component("15.rtr", "iris.router")
s_15_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """15"""
})
s_16_cpu = sst.Component("16.cpu", "portals4_sm.trig_cpu")
s_16_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """16"""
})
s_16_nic = sst.Component("16.nic", "iris.trig_nic")
s_16_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """16"""
})
s_16_rtr = sst.Component("16.rtr", "iris.router")
s_16_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """16"""
})
s_17_cpu = sst.Component("17.cpu", "portals4_sm.trig_cpu")
s_17_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """17"""
})
s_17_nic = sst.Component("17.nic", "iris.trig_nic")
s_17_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """17"""
})
s_17_rtr = sst.Component("17.rtr", "iris.router")
s_17_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """17"""
})
s_18_cpu = sst.Component("18.cpu", "portals4_sm.trig_cpu")
s_18_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """18"""
})
s_18_nic = sst.Component("18.nic", "iris.trig_nic")
s_18_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """18"""
})
s_18_rtr = sst.Component("18.rtr", "iris.router")
s_18_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """18"""
})
s_19_cpu = sst.Component("19.cpu", "portals4_sm.trig_cpu")
s_19_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """19"""
})
s_19_nic = sst.Component("19.nic", "iris.trig_nic")
s_19_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """19"""
})
s_19_rtr = sst.Component("19.rtr", "iris.router")
s_19_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """19"""
})
s_20_cpu = sst.Component("20.cpu", "portals4_sm.trig_cpu")
s_20_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """20"""
})
s_20_nic = sst.Component("20.nic", "iris.trig_nic")
s_20_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """20"""
})
s_20_rtr = sst.Component("20.rtr", "iris.router")
s_20_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """20"""
})
s_21_cpu = sst.Component("21.cpu", "portals4_sm.trig_cpu")
s_21_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """21"""
})
s_21_nic = sst.Component("21.nic", "iris.trig_nic")
s_21_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """21"""
})
s_21_rtr = sst.Component("21.rtr", "iris.router")
s_21_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """21"""
})
s_22_cpu = sst.Component("22.cpu", "portals4_sm.trig_cpu")
s_22_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """22"""
})
s_22_nic = sst.Component("22.nic", "iris.trig_nic")
s_22_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """22"""
})
s_22_rtr = sst.Component("22.rtr", "iris.router")
s_22_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """22"""
})
s_23_cpu = sst.Component("23.cpu", "portals4_sm.trig_cpu")
s_23_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """23"""
})
s_23_nic = sst.Component("23.nic", "iris.trig_nic")
s_23_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """23"""
})
s_23_rtr = sst.Component("23.rtr", "iris.router")
s_23_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """23"""
})
s_24_cpu = sst.Component("24.cpu", "portals4_sm.trig_cpu")
s_24_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """24"""
})
s_24_nic = sst.Component("24.nic", "iris.trig_nic")
s_24_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """24"""
})
s_24_rtr = sst.Component("24.rtr", "iris.router")
s_24_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """24"""
})
s_25_cpu = sst.Component("25.cpu", "portals4_sm.trig_cpu")
s_25_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """25"""
})
s_25_nic = sst.Component("25.nic", "iris.trig_nic")
s_25_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """25"""
})
s_25_rtr = sst.Component("25.rtr", "iris.router")
s_25_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """25"""
})
s_26_cpu = sst.Component("26.cpu", "portals4_sm.trig_cpu")
s_26_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """26"""
})
s_26_nic = sst.Component("26.nic", "iris.trig_nic")
s_26_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """26"""
})
s_26_rtr = sst.Component("26.rtr", "iris.router")
s_26_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """26"""
})
s_27_cpu = sst.Component("27.cpu", "portals4_sm.trig_cpu")
s_27_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """27"""
})
s_27_nic = sst.Component("27.nic", "iris.trig_nic")
s_27_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """27"""
})
s_27_rtr = sst.Component("27.rtr", "iris.router")
s_27_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """27"""
})
s_28_cpu = sst.Component("28.cpu", "portals4_sm.trig_cpu")
s_28_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """28"""
})
s_28_nic = sst.Component("28.nic", "iris.trig_nic")
s_28_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """28"""
})
s_28_rtr = sst.Component("28.rtr", "iris.router")
s_28_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """28"""
})
s_29_cpu = sst.Component("29.cpu", "portals4_sm.trig_cpu")
s_29_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """29"""
})
s_29_nic = sst.Component("29.nic", "iris.trig_nic")
s_29_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """29"""
})
s_29_rtr = sst.Component("29.rtr", "iris.router")
s_29_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """29"""
})
s_30_cpu = sst.Component("30.cpu", "portals4_sm.trig_cpu")
s_30_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """30"""
})
s_30_nic = sst.Component("30.nic", "iris.trig_nic")
s_30_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """30"""
})
s_30_rtr = sst.Component("30.rtr", "iris.router")
s_30_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """30"""
})
s_31_cpu = sst.Component("31.cpu", "portals4_sm.trig_cpu")
s_31_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """31"""
})
s_31_nic = sst.Component("31.nic", "iris.trig_nic")
s_31_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """31"""
})
s_31_rtr = sst.Component("31.rtr", "iris.router")
s_31_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """31"""
})
s_32_cpu = sst.Component("32.cpu", "portals4_sm.trig_cpu")
s_32_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """32"""
})
s_32_nic = sst.Component("32.nic", "iris.trig_nic")
s_32_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """32"""
})
s_32_rtr = sst.Component("32.rtr", "iris.router")
s_32_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """32"""
})
s_33_cpu = sst.Component("33.cpu", "portals4_sm.trig_cpu")
s_33_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """33"""
})
s_33_nic = sst.Component("33.nic", "iris.trig_nic")
s_33_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """33"""
})
s_33_rtr = sst.Component("33.rtr", "iris.router")
s_33_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """33"""
})
s_34_cpu = sst.Component("34.cpu", "portals4_sm.trig_cpu")
s_34_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """34"""
})
s_34_nic = sst.Component("34.nic", "iris.trig_nic")
s_34_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """34"""
})
s_34_rtr = sst.Component("34.rtr", "iris.router")
s_34_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """34"""
})
s_35_cpu = sst.Component("35.cpu", "portals4_sm.trig_cpu")
s_35_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """35"""
})
s_35_nic = sst.Component("35.nic", "iris.trig_nic")
s_35_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """35"""
})
s_35_rtr = sst.Component("35.rtr", "iris.router")
s_35_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """35"""
})
s_36_cpu = sst.Component("36.cpu", "portals4_sm.trig_cpu")
s_36_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """36"""
})
s_36_nic = sst.Component("36.nic", "iris.trig_nic")
s_36_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """36"""
})
s_36_rtr = sst.Component("36.rtr", "iris.router")
s_36_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """36"""
})
s_37_cpu = sst.Component("37.cpu", "portals4_sm.trig_cpu")
s_37_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """37"""
})
s_37_nic = sst.Component("37.nic", "iris.trig_nic")
s_37_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """37"""
})
s_37_rtr = sst.Component("37.rtr", "iris.router")
s_37_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """37"""
})
s_38_cpu = sst.Component("38.cpu", "portals4_sm.trig_cpu")
s_38_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """38"""
})
s_38_nic = sst.Component("38.nic", "iris.trig_nic")
s_38_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """38"""
})
s_38_rtr = sst.Component("38.rtr", "iris.router")
s_38_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """38"""
})
s_39_cpu = sst.Component("39.cpu", "portals4_sm.trig_cpu")
s_39_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """39"""
})
s_39_nic = sst.Component("39.nic", "iris.trig_nic")
s_39_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """39"""
})
s_39_rtr = sst.Component("39.rtr", "iris.router")
s_39_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """39"""
})
s_40_cpu = sst.Component("40.cpu", "portals4_sm.trig_cpu")
s_40_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """40"""
})
s_40_nic = sst.Component("40.nic", "iris.trig_nic")
s_40_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """40"""
})
s_40_rtr = sst.Component("40.rtr", "iris.router")
s_40_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """40"""
})
s_41_cpu = sst.Component("41.cpu", "portals4_sm.trig_cpu")
s_41_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """41"""
})
s_41_nic = sst.Component("41.nic", "iris.trig_nic")
s_41_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """41"""
})
s_41_rtr = sst.Component("41.rtr", "iris.router")
s_41_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """41"""
})
s_42_cpu = sst.Component("42.cpu", "portals4_sm.trig_cpu")
s_42_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """42"""
})
s_42_nic = sst.Component("42.nic", "iris.trig_nic")
s_42_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """42"""
})
s_42_rtr = sst.Component("42.rtr", "iris.router")
s_42_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """42"""
})
s_43_cpu = sst.Component("43.cpu", "portals4_sm.trig_cpu")
s_43_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """43"""
})
s_43_nic = sst.Component("43.nic", "iris.trig_nic")
s_43_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """43"""
})
s_43_rtr = sst.Component("43.rtr", "iris.router")
s_43_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """43"""
})
s_44_cpu = sst.Component("44.cpu", "portals4_sm.trig_cpu")
s_44_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """44"""
})
s_44_nic = sst.Component("44.nic", "iris.trig_nic")
s_44_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """44"""
})
s_44_rtr = sst.Component("44.rtr", "iris.router")
s_44_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """44"""
})
s_45_cpu = sst.Component("45.cpu", "portals4_sm.trig_cpu")
s_45_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """45"""
})
s_45_nic = sst.Component("45.nic", "iris.trig_nic")
s_45_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """45"""
})
s_45_rtr = sst.Component("45.rtr", "iris.router")
s_45_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """45"""
})
s_46_cpu = sst.Component("46.cpu", "portals4_sm.trig_cpu")
s_46_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """46"""
})
s_46_nic = sst.Component("46.nic", "iris.trig_nic")
s_46_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """46"""
})
s_46_rtr = sst.Component("46.rtr", "iris.router")
s_46_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """46"""
})
s_47_cpu = sst.Component("47.cpu", "portals4_sm.trig_cpu")
s_47_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """47"""
})
s_47_nic = sst.Component("47.nic", "iris.trig_nic")
s_47_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """47"""
})
s_47_rtr = sst.Component("47.rtr", "iris.router")
s_47_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """47"""
})
s_48_cpu = sst.Component("48.cpu", "portals4_sm.trig_cpu")
s_48_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """48"""
})
s_48_nic = sst.Component("48.nic", "iris.trig_nic")
s_48_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """48"""
})
s_48_rtr = sst.Component("48.rtr", "iris.router")
s_48_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """48"""
})
s_49_cpu = sst.Component("49.cpu", "portals4_sm.trig_cpu")
s_49_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """49"""
})
s_49_nic = sst.Component("49.nic", "iris.trig_nic")
s_49_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """49"""
})
s_49_rtr = sst.Component("49.rtr", "iris.router")
s_49_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """49"""
})
s_50_cpu = sst.Component("50.cpu", "portals4_sm.trig_cpu")
s_50_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """50"""
})
s_50_nic = sst.Component("50.nic", "iris.trig_nic")
s_50_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """50"""
})
s_50_rtr = sst.Component("50.rtr", "iris.router")
s_50_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """50"""
})
s_51_cpu = sst.Component("51.cpu", "portals4_sm.trig_cpu")
s_51_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """51"""
})
s_51_nic = sst.Component("51.nic", "iris.trig_nic")
s_51_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """51"""
})
s_51_rtr = sst.Component("51.rtr", "iris.router")
s_51_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """51"""
})
s_52_cpu = sst.Component("52.cpu", "portals4_sm.trig_cpu")
s_52_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """52"""
})
s_52_nic = sst.Component("52.nic", "iris.trig_nic")
s_52_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """52"""
})
s_52_rtr = sst.Component("52.rtr", "iris.router")
s_52_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """52"""
})
s_53_cpu = sst.Component("53.cpu", "portals4_sm.trig_cpu")
s_53_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """53"""
})
s_53_nic = sst.Component("53.nic", "iris.trig_nic")
s_53_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """53"""
})
s_53_rtr = sst.Component("53.rtr", "iris.router")
s_53_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """53"""
})
s_54_cpu = sst.Component("54.cpu", "portals4_sm.trig_cpu")
s_54_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """54"""
})
s_54_nic = sst.Component("54.nic", "iris.trig_nic")
s_54_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """54"""
})
s_54_rtr = sst.Component("54.rtr", "iris.router")
s_54_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """54"""
})
s_55_cpu = sst.Component("55.cpu", "portals4_sm.trig_cpu")
s_55_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """55"""
})
s_55_nic = sst.Component("55.nic", "iris.trig_nic")
s_55_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """55"""
})
s_55_rtr = sst.Component("55.rtr", "iris.router")
s_55_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """55"""
})
s_56_cpu = sst.Component("56.cpu", "portals4_sm.trig_cpu")
s_56_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """56"""
})
s_56_nic = sst.Component("56.nic", "iris.trig_nic")
s_56_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """56"""
})
s_56_rtr = sst.Component("56.rtr", "iris.router")
s_56_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """56"""
})
s_57_cpu = sst.Component("57.cpu", "portals4_sm.trig_cpu")
s_57_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """57"""
})
s_57_nic = sst.Component("57.nic", "iris.trig_nic")
s_57_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """57"""
})
s_57_rtr = sst.Component("57.rtr", "iris.router")
s_57_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """57"""
})
s_58_cpu = sst.Component("58.cpu", "portals4_sm.trig_cpu")
s_58_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """58"""
})
s_58_nic = sst.Component("58.nic", "iris.trig_nic")
s_58_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """58"""
})
s_58_rtr = sst.Component("58.rtr", "iris.router")
s_58_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """58"""
})
s_59_cpu = sst.Component("59.cpu", "portals4_sm.trig_cpu")
s_59_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """59"""
})
s_59_nic = sst.Component("59.nic", "iris.trig_nic")
s_59_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """59"""
})
s_59_rtr = sst.Component("59.rtr", "iris.router")
s_59_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """59"""
})
s_60_cpu = sst.Component("60.cpu", "portals4_sm.trig_cpu")
s_60_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """60"""
})
s_60_nic = sst.Component("60.nic", "iris.trig_nic")
s_60_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """60"""
})
s_60_rtr = sst.Component("60.rtr", "iris.router")
s_60_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """60"""
})
s_61_cpu = sst.Component("61.cpu", "portals4_sm.trig_cpu")
s_61_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """61"""
})
s_61_nic = sst.Component("61.nic", "iris.trig_nic")
s_61_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """61"""
})
s_61_rtr = sst.Component("61.rtr", "iris.router")
s_61_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """61"""
})
s_62_cpu = sst.Component("62.cpu", "portals4_sm.trig_cpu")
s_62_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """62"""
})
s_62_nic = sst.Component("62.nic", "iris.trig_nic")
s_62_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """62"""
})
s_62_rtr = sst.Component("62.rtr", "iris.router")
s_62_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """62"""
})
s_63_cpu = sst.Component("63.cpu", "portals4_sm.trig_cpu")
s_63_cpu.addParams({
      "radix" : """4""",
      "timing_set" : """2""",
      "nodes" : """64""",
      "msgrate" : """5MHz""",
      "xDimSize" : """8""",
      "yDimSize" : """8""",
      "zDimSize" : """1""",
      "noiseRuns" : """0""",
      "noiseInterval" : """1kHz""",
      "noiseDuration" : """25us""",
      "application" : """allreduce.tree_triggered""",
      "latency" : """500""",
      "msg_size" : """1048576""",
      "chunk_size" : """16384""",
      "coalesce" : """1""",
      "enable_putv" : """0""",
      "id" : """63"""
})
s_63_nic = sst.Component("63.nic", "iris.trig_nic")
s_63_nic.addParams({
      "timing_set" : """2""",
      "latency" : """500""",
      "clock" : """500Mhz""",
      "info" : """no""",
      "debug" : """no""",
      "dummyDebug" : """no""",
      "id" : """63"""
})
s_63_rtr = sst.Component("63.rtr", "iris.router")
s_63_rtr.addParams({
      "credits" : """30""",
      "ports" : """7""",
      "vcs" : """4""",
      "no_nodes" : """64""",
      "grid_size" : """8""",
      "id" : """63"""
})


# Define the simulation links
s_0_cpu2nic = sst.Link("s_0_cpu2nic")
s_0_cpu2nic.connect( (s_0_cpu, "nic", "200000ps"), (s_0_nic, "cpu", "200000ps") )
s_0_nic2rtr = sst.Link("s_0_nic2rtr")
s_0_nic2rtr.connect( (s_0_nic, "rtr", "200000ps"), (s_0_rtr, "nic", "200000ps") )
s_1_cpu2nic = sst.Link("s_1_cpu2nic")
s_1_cpu2nic.connect( (s_1_cpu, "nic", "200000ps"), (s_1_nic, "cpu", "200000ps") )
s_1_nic2rtr = sst.Link("s_1_nic2rtr")
s_1_nic2rtr.connect( (s_1_nic, "rtr", "200000ps"), (s_1_rtr, "nic", "200000ps") )
s_10_cpu2nic = sst.Link("s_10_cpu2nic")
s_10_cpu2nic.connect( (s_10_cpu, "nic", "200000ps"), (s_10_nic, "cpu", "200000ps") )
s_10_nic2rtr = sst.Link("s_10_nic2rtr")
s_10_nic2rtr.connect( (s_10_nic, "rtr", "200000ps"), (s_10_rtr, "nic", "200000ps") )
s_11_cpu2nic = sst.Link("s_11_cpu2nic")
s_11_cpu2nic.connect( (s_11_cpu, "nic", "200000ps"), (s_11_nic, "cpu", "200000ps") )
s_11_nic2rtr = sst.Link("s_11_nic2rtr")
s_11_nic2rtr.connect( (s_11_nic, "rtr", "200000ps"), (s_11_rtr, "nic", "200000ps") )
s_12_cpu2nic = sst.Link("s_12_cpu2nic")
s_12_cpu2nic.connect( (s_12_cpu, "nic", "200000ps"), (s_12_nic, "cpu", "200000ps") )
s_12_nic2rtr = sst.Link("s_12_nic2rtr")
s_12_nic2rtr.connect( (s_12_nic, "rtr", "200000ps"), (s_12_rtr, "nic", "200000ps") )
s_13_cpu2nic = sst.Link("s_13_cpu2nic")
s_13_cpu2nic.connect( (s_13_cpu, "nic", "200000ps"), (s_13_nic, "cpu", "200000ps") )
s_13_nic2rtr = sst.Link("s_13_nic2rtr")
s_13_nic2rtr.connect( (s_13_nic, "rtr", "200000ps"), (s_13_rtr, "nic", "200000ps") )
s_14_cpu2nic = sst.Link("s_14_cpu2nic")
s_14_cpu2nic.connect( (s_14_cpu, "nic", "200000ps"), (s_14_nic, "cpu", "200000ps") )
s_14_nic2rtr = sst.Link("s_14_nic2rtr")
s_14_nic2rtr.connect( (s_14_nic, "rtr", "200000ps"), (s_14_rtr, "nic", "200000ps") )
s_15_cpu2nic = sst.Link("s_15_cpu2nic")
s_15_cpu2nic.connect( (s_15_cpu, "nic", "200000ps"), (s_15_nic, "cpu", "200000ps") )
s_15_nic2rtr = sst.Link("s_15_nic2rtr")
s_15_nic2rtr.connect( (s_15_nic, "rtr", "200000ps"), (s_15_rtr, "nic", "200000ps") )
s_16_cpu2nic = sst.Link("s_16_cpu2nic")
s_16_cpu2nic.connect( (s_16_cpu, "nic", "200000ps"), (s_16_nic, "cpu", "200000ps") )
s_16_nic2rtr = sst.Link("s_16_nic2rtr")
s_16_nic2rtr.connect( (s_16_nic, "rtr", "200000ps"), (s_16_rtr, "nic", "200000ps") )
s_17_cpu2nic = sst.Link("s_17_cpu2nic")
s_17_cpu2nic.connect( (s_17_cpu, "nic", "200000ps"), (s_17_nic, "cpu", "200000ps") )
s_17_nic2rtr = sst.Link("s_17_nic2rtr")
s_17_nic2rtr.connect( (s_17_nic, "rtr", "200000ps"), (s_17_rtr, "nic", "200000ps") )
s_18_cpu2nic = sst.Link("s_18_cpu2nic")
s_18_cpu2nic.connect( (s_18_cpu, "nic", "200000ps"), (s_18_nic, "cpu", "200000ps") )
s_18_nic2rtr = sst.Link("s_18_nic2rtr")
s_18_nic2rtr.connect( (s_18_nic, "rtr", "200000ps"), (s_18_rtr, "nic", "200000ps") )
s_19_cpu2nic = sst.Link("s_19_cpu2nic")
s_19_cpu2nic.connect( (s_19_cpu, "nic", "200000ps"), (s_19_nic, "cpu", "200000ps") )
s_19_nic2rtr = sst.Link("s_19_nic2rtr")
s_19_nic2rtr.connect( (s_19_nic, "rtr", "200000ps"), (s_19_rtr, "nic", "200000ps") )
s_2_cpu2nic = sst.Link("s_2_cpu2nic")
s_2_cpu2nic.connect( (s_2_cpu, "nic", "200000ps"), (s_2_nic, "cpu", "200000ps") )
s_2_nic2rtr = sst.Link("s_2_nic2rtr")
s_2_nic2rtr.connect( (s_2_nic, "rtr", "200000ps"), (s_2_rtr, "nic", "200000ps") )
s_20_cpu2nic = sst.Link("s_20_cpu2nic")
s_20_cpu2nic.connect( (s_20_cpu, "nic", "200000ps"), (s_20_nic, "cpu", "200000ps") )
s_20_nic2rtr = sst.Link("s_20_nic2rtr")
s_20_nic2rtr.connect( (s_20_nic, "rtr", "200000ps"), (s_20_rtr, "nic", "200000ps") )
s_21_cpu2nic = sst.Link("s_21_cpu2nic")
s_21_cpu2nic.connect( (s_21_cpu, "nic", "200000ps"), (s_21_nic, "cpu", "200000ps") )
s_21_nic2rtr = sst.Link("s_21_nic2rtr")
s_21_nic2rtr.connect( (s_21_nic, "rtr", "200000ps"), (s_21_rtr, "nic", "200000ps") )
s_22_cpu2nic = sst.Link("s_22_cpu2nic")
s_22_cpu2nic.connect( (s_22_cpu, "nic", "200000ps"), (s_22_nic, "cpu", "200000ps") )
s_22_nic2rtr = sst.Link("s_22_nic2rtr")
s_22_nic2rtr.connect( (s_22_nic, "rtr", "200000ps"), (s_22_rtr, "nic", "200000ps") )
s_23_cpu2nic = sst.Link("s_23_cpu2nic")
s_23_cpu2nic.connect( (s_23_cpu, "nic", "200000ps"), (s_23_nic, "cpu", "200000ps") )
s_23_nic2rtr = sst.Link("s_23_nic2rtr")
s_23_nic2rtr.connect( (s_23_nic, "rtr", "200000ps"), (s_23_rtr, "nic", "200000ps") )
s_24_cpu2nic = sst.Link("s_24_cpu2nic")
s_24_cpu2nic.connect( (s_24_cpu, "nic", "200000ps"), (s_24_nic, "cpu", "200000ps") )
s_24_nic2rtr = sst.Link("s_24_nic2rtr")
s_24_nic2rtr.connect( (s_24_nic, "rtr", "200000ps"), (s_24_rtr, "nic", "200000ps") )
s_25_cpu2nic = sst.Link("s_25_cpu2nic")
s_25_cpu2nic.connect( (s_25_cpu, "nic", "200000ps"), (s_25_nic, "cpu", "200000ps") )
s_25_nic2rtr = sst.Link("s_25_nic2rtr")
s_25_nic2rtr.connect( (s_25_nic, "rtr", "200000ps"), (s_25_rtr, "nic", "200000ps") )
s_26_cpu2nic = sst.Link("s_26_cpu2nic")
s_26_cpu2nic.connect( (s_26_cpu, "nic", "200000ps"), (s_26_nic, "cpu", "200000ps") )
s_26_nic2rtr = sst.Link("s_26_nic2rtr")
s_26_nic2rtr.connect( (s_26_nic, "rtr", "200000ps"), (s_26_rtr, "nic", "200000ps") )
s_27_cpu2nic = sst.Link("s_27_cpu2nic")
s_27_cpu2nic.connect( (s_27_cpu, "nic", "200000ps"), (s_27_nic, "cpu", "200000ps") )
s_27_nic2rtr = sst.Link("s_27_nic2rtr")
s_27_nic2rtr.connect( (s_27_nic, "rtr", "200000ps"), (s_27_rtr, "nic", "200000ps") )
s_28_cpu2nic = sst.Link("s_28_cpu2nic")
s_28_cpu2nic.connect( (s_28_cpu, "nic", "200000ps"), (s_28_nic, "cpu", "200000ps") )
s_28_nic2rtr = sst.Link("s_28_nic2rtr")
s_28_nic2rtr.connect( (s_28_nic, "rtr", "200000ps"), (s_28_rtr, "nic", "200000ps") )
s_29_cpu2nic = sst.Link("s_29_cpu2nic")
s_29_cpu2nic.connect( (s_29_cpu, "nic", "200000ps"), (s_29_nic, "cpu", "200000ps") )
s_29_nic2rtr = sst.Link("s_29_nic2rtr")
s_29_nic2rtr.connect( (s_29_nic, "rtr", "200000ps"), (s_29_rtr, "nic", "200000ps") )
s_3_cpu2nic = sst.Link("s_3_cpu2nic")
s_3_cpu2nic.connect( (s_3_cpu, "nic", "200000ps"), (s_3_nic, "cpu", "200000ps") )
s_3_nic2rtr = sst.Link("s_3_nic2rtr")
s_3_nic2rtr.connect( (s_3_nic, "rtr", "200000ps"), (s_3_rtr, "nic", "200000ps") )
s_30_cpu2nic = sst.Link("s_30_cpu2nic")
s_30_cpu2nic.connect( (s_30_cpu, "nic", "200000ps"), (s_30_nic, "cpu", "200000ps") )
s_30_nic2rtr = sst.Link("s_30_nic2rtr")
s_30_nic2rtr.connect( (s_30_nic, "rtr", "200000ps"), (s_30_rtr, "nic", "200000ps") )
s_31_cpu2nic = sst.Link("s_31_cpu2nic")
s_31_cpu2nic.connect( (s_31_cpu, "nic", "200000ps"), (s_31_nic, "cpu", "200000ps") )
s_31_nic2rtr = sst.Link("s_31_nic2rtr")
s_31_nic2rtr.connect( (s_31_nic, "rtr", "200000ps"), (s_31_rtr, "nic", "200000ps") )
s_32_cpu2nic = sst.Link("s_32_cpu2nic")
s_32_cpu2nic.connect( (s_32_cpu, "nic", "200000ps"), (s_32_nic, "cpu", "200000ps") )
s_32_nic2rtr = sst.Link("s_32_nic2rtr")
s_32_nic2rtr.connect( (s_32_nic, "rtr", "200000ps"), (s_32_rtr, "nic", "200000ps") )
s_33_cpu2nic = sst.Link("s_33_cpu2nic")
s_33_cpu2nic.connect( (s_33_cpu, "nic", "200000ps"), (s_33_nic, "cpu", "200000ps") )
s_33_nic2rtr = sst.Link("s_33_nic2rtr")
s_33_nic2rtr.connect( (s_33_nic, "rtr", "200000ps"), (s_33_rtr, "nic", "200000ps") )
s_34_cpu2nic = sst.Link("s_34_cpu2nic")
s_34_cpu2nic.connect( (s_34_cpu, "nic", "200000ps"), (s_34_nic, "cpu", "200000ps") )
s_34_nic2rtr = sst.Link("s_34_nic2rtr")
s_34_nic2rtr.connect( (s_34_nic, "rtr", "200000ps"), (s_34_rtr, "nic", "200000ps") )
s_35_cpu2nic = sst.Link("s_35_cpu2nic")
s_35_cpu2nic.connect( (s_35_cpu, "nic", "200000ps"), (s_35_nic, "cpu", "200000ps") )
s_35_nic2rtr = sst.Link("s_35_nic2rtr")
s_35_nic2rtr.connect( (s_35_nic, "rtr", "200000ps"), (s_35_rtr, "nic", "200000ps") )
s_36_cpu2nic = sst.Link("s_36_cpu2nic")
s_36_cpu2nic.connect( (s_36_cpu, "nic", "200000ps"), (s_36_nic, "cpu", "200000ps") )
s_36_nic2rtr = sst.Link("s_36_nic2rtr")
s_36_nic2rtr.connect( (s_36_nic, "rtr", "200000ps"), (s_36_rtr, "nic", "200000ps") )
s_37_cpu2nic = sst.Link("s_37_cpu2nic")
s_37_cpu2nic.connect( (s_37_cpu, "nic", "200000ps"), (s_37_nic, "cpu", "200000ps") )
s_37_nic2rtr = sst.Link("s_37_nic2rtr")
s_37_nic2rtr.connect( (s_37_nic, "rtr", "200000ps"), (s_37_rtr, "nic", "200000ps") )
s_38_cpu2nic = sst.Link("s_38_cpu2nic")
s_38_cpu2nic.connect( (s_38_cpu, "nic", "200000ps"), (s_38_nic, "cpu", "200000ps") )
s_38_nic2rtr = sst.Link("s_38_nic2rtr")
s_38_nic2rtr.connect( (s_38_nic, "rtr", "200000ps"), (s_38_rtr, "nic", "200000ps") )
s_39_cpu2nic = sst.Link("s_39_cpu2nic")
s_39_cpu2nic.connect( (s_39_cpu, "nic", "200000ps"), (s_39_nic, "cpu", "200000ps") )
s_39_nic2rtr = sst.Link("s_39_nic2rtr")
s_39_nic2rtr.connect( (s_39_nic, "rtr", "200000ps"), (s_39_rtr, "nic", "200000ps") )
s_4_cpu2nic = sst.Link("s_4_cpu2nic")
s_4_cpu2nic.connect( (s_4_cpu, "nic", "200000ps"), (s_4_nic, "cpu", "200000ps") )
s_4_nic2rtr = sst.Link("s_4_nic2rtr")
s_4_nic2rtr.connect( (s_4_nic, "rtr", "200000ps"), (s_4_rtr, "nic", "200000ps") )
s_40_cpu2nic = sst.Link("s_40_cpu2nic")
s_40_cpu2nic.connect( (s_40_cpu, "nic", "200000ps"), (s_40_nic, "cpu", "200000ps") )
s_40_nic2rtr = sst.Link("s_40_nic2rtr")
s_40_nic2rtr.connect( (s_40_nic, "rtr", "200000ps"), (s_40_rtr, "nic", "200000ps") )
s_41_cpu2nic = sst.Link("s_41_cpu2nic")
s_41_cpu2nic.connect( (s_41_cpu, "nic", "200000ps"), (s_41_nic, "cpu", "200000ps") )
s_41_nic2rtr = sst.Link("s_41_nic2rtr")
s_41_nic2rtr.connect( (s_41_nic, "rtr", "200000ps"), (s_41_rtr, "nic", "200000ps") )
s_42_cpu2nic = sst.Link("s_42_cpu2nic")
s_42_cpu2nic.connect( (s_42_cpu, "nic", "200000ps"), (s_42_nic, "cpu", "200000ps") )
s_42_nic2rtr = sst.Link("s_42_nic2rtr")
s_42_nic2rtr.connect( (s_42_nic, "rtr", "200000ps"), (s_42_rtr, "nic", "200000ps") )
s_43_cpu2nic = sst.Link("s_43_cpu2nic")
s_43_cpu2nic.connect( (s_43_cpu, "nic", "200000ps"), (s_43_nic, "cpu", "200000ps") )
s_43_nic2rtr = sst.Link("s_43_nic2rtr")
s_43_nic2rtr.connect( (s_43_nic, "rtr", "200000ps"), (s_43_rtr, "nic", "200000ps") )
s_44_cpu2nic = sst.Link("s_44_cpu2nic")
s_44_cpu2nic.connect( (s_44_cpu, "nic", "200000ps"), (s_44_nic, "cpu", "200000ps") )
s_44_nic2rtr = sst.Link("s_44_nic2rtr")
s_44_nic2rtr.connect( (s_44_nic, "rtr", "200000ps"), (s_44_rtr, "nic", "200000ps") )
s_45_cpu2nic = sst.Link("s_45_cpu2nic")
s_45_cpu2nic.connect( (s_45_cpu, "nic", "200000ps"), (s_45_nic, "cpu", "200000ps") )
s_45_nic2rtr = sst.Link("s_45_nic2rtr")
s_45_nic2rtr.connect( (s_45_nic, "rtr", "200000ps"), (s_45_rtr, "nic", "200000ps") )
s_46_cpu2nic = sst.Link("s_46_cpu2nic")
s_46_cpu2nic.connect( (s_46_cpu, "nic", "200000ps"), (s_46_nic, "cpu", "200000ps") )
s_46_nic2rtr = sst.Link("s_46_nic2rtr")
s_46_nic2rtr.connect( (s_46_nic, "rtr", "200000ps"), (s_46_rtr, "nic", "200000ps") )
s_47_cpu2nic = sst.Link("s_47_cpu2nic")
s_47_cpu2nic.connect( (s_47_cpu, "nic", "200000ps"), (s_47_nic, "cpu", "200000ps") )
s_47_nic2rtr = sst.Link("s_47_nic2rtr")
s_47_nic2rtr.connect( (s_47_nic, "rtr", "200000ps"), (s_47_rtr, "nic", "200000ps") )
s_48_cpu2nic = sst.Link("s_48_cpu2nic")
s_48_cpu2nic.connect( (s_48_cpu, "nic", "200000ps"), (s_48_nic, "cpu", "200000ps") )
s_48_nic2rtr = sst.Link("s_48_nic2rtr")
s_48_nic2rtr.connect( (s_48_nic, "rtr", "200000ps"), (s_48_rtr, "nic", "200000ps") )
s_49_cpu2nic = sst.Link("s_49_cpu2nic")
s_49_cpu2nic.connect( (s_49_cpu, "nic", "200000ps"), (s_49_nic, "cpu", "200000ps") )
s_49_nic2rtr = sst.Link("s_49_nic2rtr")
s_49_nic2rtr.connect( (s_49_nic, "rtr", "200000ps"), (s_49_rtr, "nic", "200000ps") )
s_5_cpu2nic = sst.Link("s_5_cpu2nic")
s_5_cpu2nic.connect( (s_5_cpu, "nic", "200000ps"), (s_5_nic, "cpu", "200000ps") )
s_5_nic2rtr = sst.Link("s_5_nic2rtr")
s_5_nic2rtr.connect( (s_5_nic, "rtr", "200000ps"), (s_5_rtr, "nic", "200000ps") )
s_50_cpu2nic = sst.Link("s_50_cpu2nic")
s_50_cpu2nic.connect( (s_50_cpu, "nic", "200000ps"), (s_50_nic, "cpu", "200000ps") )
s_50_nic2rtr = sst.Link("s_50_nic2rtr")
s_50_nic2rtr.connect( (s_50_nic, "rtr", "200000ps"), (s_50_rtr, "nic", "200000ps") )
s_51_cpu2nic = sst.Link("s_51_cpu2nic")
s_51_cpu2nic.connect( (s_51_cpu, "nic", "200000ps"), (s_51_nic, "cpu", "200000ps") )
s_51_nic2rtr = sst.Link("s_51_nic2rtr")
s_51_nic2rtr.connect( (s_51_nic, "rtr", "200000ps"), (s_51_rtr, "nic", "200000ps") )
s_52_cpu2nic = sst.Link("s_52_cpu2nic")
s_52_cpu2nic.connect( (s_52_cpu, "nic", "200000ps"), (s_52_nic, "cpu", "200000ps") )
s_52_nic2rtr = sst.Link("s_52_nic2rtr")
s_52_nic2rtr.connect( (s_52_nic, "rtr", "200000ps"), (s_52_rtr, "nic", "200000ps") )
s_53_cpu2nic = sst.Link("s_53_cpu2nic")
s_53_cpu2nic.connect( (s_53_cpu, "nic", "200000ps"), (s_53_nic, "cpu", "200000ps") )
s_53_nic2rtr = sst.Link("s_53_nic2rtr")
s_53_nic2rtr.connect( (s_53_nic, "rtr", "200000ps"), (s_53_rtr, "nic", "200000ps") )
s_54_cpu2nic = sst.Link("s_54_cpu2nic")
s_54_cpu2nic.connect( (s_54_cpu, "nic", "200000ps"), (s_54_nic, "cpu", "200000ps") )
s_54_nic2rtr = sst.Link("s_54_nic2rtr")
s_54_nic2rtr.connect( (s_54_nic, "rtr", "200000ps"), (s_54_rtr, "nic", "200000ps") )
s_55_cpu2nic = sst.Link("s_55_cpu2nic")
s_55_cpu2nic.connect( (s_55_cpu, "nic", "200000ps"), (s_55_nic, "cpu", "200000ps") )
s_55_nic2rtr = sst.Link("s_55_nic2rtr")
s_55_nic2rtr.connect( (s_55_nic, "rtr", "200000ps"), (s_55_rtr, "nic", "200000ps") )
s_56_cpu2nic = sst.Link("s_56_cpu2nic")
s_56_cpu2nic.connect( (s_56_cpu, "nic", "200000ps"), (s_56_nic, "cpu", "200000ps") )
s_56_nic2rtr = sst.Link("s_56_nic2rtr")
s_56_nic2rtr.connect( (s_56_nic, "rtr", "200000ps"), (s_56_rtr, "nic", "200000ps") )
s_57_cpu2nic = sst.Link("s_57_cpu2nic")
s_57_cpu2nic.connect( (s_57_cpu, "nic", "200000ps"), (s_57_nic, "cpu", "200000ps") )
s_57_nic2rtr = sst.Link("s_57_nic2rtr")
s_57_nic2rtr.connect( (s_57_nic, "rtr", "200000ps"), (s_57_rtr, "nic", "200000ps") )
s_58_cpu2nic = sst.Link("s_58_cpu2nic")
s_58_cpu2nic.connect( (s_58_cpu, "nic", "200000ps"), (s_58_nic, "cpu", "200000ps") )
s_58_nic2rtr = sst.Link("s_58_nic2rtr")
s_58_nic2rtr.connect( (s_58_nic, "rtr", "200000ps"), (s_58_rtr, "nic", "200000ps") )
s_59_cpu2nic = sst.Link("s_59_cpu2nic")
s_59_cpu2nic.connect( (s_59_cpu, "nic", "200000ps"), (s_59_nic, "cpu", "200000ps") )
s_59_nic2rtr = sst.Link("s_59_nic2rtr")
s_59_nic2rtr.connect( (s_59_nic, "rtr", "200000ps"), (s_59_rtr, "nic", "200000ps") )
s_6_cpu2nic = sst.Link("s_6_cpu2nic")
s_6_cpu2nic.connect( (s_6_cpu, "nic", "200000ps"), (s_6_nic, "cpu", "200000ps") )
s_6_nic2rtr = sst.Link("s_6_nic2rtr")
s_6_nic2rtr.connect( (s_6_nic, "rtr", "200000ps"), (s_6_rtr, "nic", "200000ps") )
s_60_cpu2nic = sst.Link("s_60_cpu2nic")
s_60_cpu2nic.connect( (s_60_cpu, "nic", "200000ps"), (s_60_nic, "cpu", "200000ps") )
s_60_nic2rtr = sst.Link("s_60_nic2rtr")
s_60_nic2rtr.connect( (s_60_nic, "rtr", "200000ps"), (s_60_rtr, "nic", "200000ps") )
s_61_cpu2nic = sst.Link("s_61_cpu2nic")
s_61_cpu2nic.connect( (s_61_cpu, "nic", "200000ps"), (s_61_nic, "cpu", "200000ps") )
s_61_nic2rtr = sst.Link("s_61_nic2rtr")
s_61_nic2rtr.connect( (s_61_nic, "rtr", "200000ps"), (s_61_rtr, "nic", "200000ps") )
s_62_cpu2nic = sst.Link("s_62_cpu2nic")
s_62_cpu2nic.connect( (s_62_cpu, "nic", "200000ps"), (s_62_nic, "cpu", "200000ps") )
s_62_nic2rtr = sst.Link("s_62_nic2rtr")
s_62_nic2rtr.connect( (s_62_nic, "rtr", "200000ps"), (s_62_rtr, "nic", "200000ps") )
s_63_cpu2nic = sst.Link("s_63_cpu2nic")
s_63_cpu2nic.connect( (s_63_cpu, "nic", "200000ps"), (s_63_nic, "cpu", "200000ps") )
s_63_nic2rtr = sst.Link("s_63_nic2rtr")
s_63_nic2rtr.connect( (s_63_nic, "rtr", "200000ps"), (s_63_rtr, "nic", "200000ps") )
s_7_cpu2nic = sst.Link("s_7_cpu2nic")
s_7_cpu2nic.connect( (s_7_cpu, "nic", "200000ps"), (s_7_nic, "cpu", "200000ps") )
s_7_nic2rtr = sst.Link("s_7_nic2rtr")
s_7_nic2rtr.connect( (s_7_nic, "rtr", "200000ps"), (s_7_rtr, "nic", "200000ps") )
s_8_cpu2nic = sst.Link("s_8_cpu2nic")
s_8_cpu2nic.connect( (s_8_cpu, "nic", "200000ps"), (s_8_nic, "cpu", "200000ps") )
s_8_nic2rtr = sst.Link("s_8_nic2rtr")
s_8_nic2rtr.connect( (s_8_nic, "rtr", "200000ps"), (s_8_rtr, "nic", "200000ps") )
s_9_cpu2nic = sst.Link("s_9_cpu2nic")
s_9_cpu2nic.connect( (s_9_cpu, "nic", "200000ps"), (s_9_nic, "cpu", "200000ps") )
s_9_nic2rtr = sst.Link("s_9_nic2rtr")
s_9_nic2rtr.connect( (s_9_nic, "rtr", "200000ps"), (s_9_rtr, "nic", "200000ps") )
xr2r_0_0_0 = sst.Link("xr2r_0_0_0")
xr2r_0_0_0.connect( (s_0_rtr, "xNeg", "10000ps"), (s_7_rtr, "xPos", "10000ps") )
xr2r_0_0_1 = sst.Link("xr2r_0_0_1")
xr2r_0_0_1.connect( (s_0_rtr, "xPos", "10000ps"), (s_1_rtr, "xNeg", "10000ps") )
xr2r_0_0_2 = sst.Link("xr2r_0_0_2")
xr2r_0_0_2.connect( (s_1_rtr, "xPos", "10000ps"), (s_2_rtr, "xNeg", "10000ps") )
xr2r_0_0_3 = sst.Link("xr2r_0_0_3")
xr2r_0_0_3.connect( (s_2_rtr, "xPos", "10000ps"), (s_3_rtr, "xNeg", "10000ps") )
xr2r_0_0_4 = sst.Link("xr2r_0_0_4")
xr2r_0_0_4.connect( (s_3_rtr, "xPos", "10000ps"), (s_4_rtr, "xNeg", "10000ps") )
xr2r_0_0_5 = sst.Link("xr2r_0_0_5")
xr2r_0_0_5.connect( (s_4_rtr, "xPos", "10000ps"), (s_5_rtr, "xNeg", "10000ps") )
xr2r_0_0_6 = sst.Link("xr2r_0_0_6")
xr2r_0_0_6.connect( (s_5_rtr, "xPos", "10000ps"), (s_6_rtr, "xNeg", "10000ps") )
xr2r_0_0_7 = sst.Link("xr2r_0_0_7")
xr2r_0_0_7.connect( (s_6_rtr, "xPos", "10000ps"), (s_7_rtr, "xNeg", "10000ps") )
xr2r_1_0_0 = sst.Link("xr2r_1_0_0")
xr2r_1_0_0.connect( (s_8_rtr, "xNeg", "10000ps"), (s_15_rtr, "xPos", "10000ps") )
xr2r_1_0_1 = sst.Link("xr2r_1_0_1")
xr2r_1_0_1.connect( (s_8_rtr, "xPos", "10000ps"), (s_9_rtr, "xNeg", "10000ps") )
xr2r_1_0_2 = sst.Link("xr2r_1_0_2")
xr2r_1_0_2.connect( (s_9_rtr, "xPos", "10000ps"), (s_10_rtr, "xNeg", "10000ps") )
xr2r_1_0_3 = sst.Link("xr2r_1_0_3")
xr2r_1_0_3.connect( (s_10_rtr, "xPos", "10000ps"), (s_11_rtr, "xNeg", "10000ps") )
xr2r_1_0_4 = sst.Link("xr2r_1_0_4")
xr2r_1_0_4.connect( (s_11_rtr, "xPos", "10000ps"), (s_12_rtr, "xNeg", "10000ps") )
xr2r_1_0_5 = sst.Link("xr2r_1_0_5")
xr2r_1_0_5.connect( (s_12_rtr, "xPos", "10000ps"), (s_13_rtr, "xNeg", "10000ps") )
xr2r_1_0_6 = sst.Link("xr2r_1_0_6")
xr2r_1_0_6.connect( (s_13_rtr, "xPos", "10000ps"), (s_14_rtr, "xNeg", "10000ps") )
xr2r_1_0_7 = sst.Link("xr2r_1_0_7")
xr2r_1_0_7.connect( (s_14_rtr, "xPos", "10000ps"), (s_15_rtr, "xNeg", "10000ps") )
xr2r_2_0_0 = sst.Link("xr2r_2_0_0")
xr2r_2_0_0.connect( (s_16_rtr, "xNeg", "10000ps"), (s_23_rtr, "xPos", "10000ps") )
xr2r_2_0_1 = sst.Link("xr2r_2_0_1")
xr2r_2_0_1.connect( (s_16_rtr, "xPos", "10000ps"), (s_17_rtr, "xNeg", "10000ps") )
xr2r_2_0_2 = sst.Link("xr2r_2_0_2")
xr2r_2_0_2.connect( (s_17_rtr, "xPos", "10000ps"), (s_18_rtr, "xNeg", "10000ps") )
xr2r_2_0_3 = sst.Link("xr2r_2_0_3")
xr2r_2_0_3.connect( (s_18_rtr, "xPos", "10000ps"), (s_19_rtr, "xNeg", "10000ps") )
xr2r_2_0_4 = sst.Link("xr2r_2_0_4")
xr2r_2_0_4.connect( (s_19_rtr, "xPos", "10000ps"), (s_20_rtr, "xNeg", "10000ps") )
xr2r_2_0_5 = sst.Link("xr2r_2_0_5")
xr2r_2_0_5.connect( (s_20_rtr, "xPos", "10000ps"), (s_21_rtr, "xNeg", "10000ps") )
xr2r_2_0_6 = sst.Link("xr2r_2_0_6")
xr2r_2_0_6.connect( (s_21_rtr, "xPos", "10000ps"), (s_22_rtr, "xNeg", "10000ps") )
xr2r_2_0_7 = sst.Link("xr2r_2_0_7")
xr2r_2_0_7.connect( (s_22_rtr, "xPos", "10000ps"), (s_23_rtr, "xNeg", "10000ps") )
xr2r_3_0_0 = sst.Link("xr2r_3_0_0")
xr2r_3_0_0.connect( (s_24_rtr, "xNeg", "10000ps"), (s_31_rtr, "xPos", "10000ps") )
xr2r_3_0_1 = sst.Link("xr2r_3_0_1")
xr2r_3_0_1.connect( (s_24_rtr, "xPos", "10000ps"), (s_25_rtr, "xNeg", "10000ps") )
xr2r_3_0_2 = sst.Link("xr2r_3_0_2")
xr2r_3_0_2.connect( (s_25_rtr, "xPos", "10000ps"), (s_26_rtr, "xNeg", "10000ps") )
xr2r_3_0_3 = sst.Link("xr2r_3_0_3")
xr2r_3_0_3.connect( (s_26_rtr, "xPos", "10000ps"), (s_27_rtr, "xNeg", "10000ps") )
xr2r_3_0_4 = sst.Link("xr2r_3_0_4")
xr2r_3_0_4.connect( (s_27_rtr, "xPos", "10000ps"), (s_28_rtr, "xNeg", "10000ps") )
xr2r_3_0_5 = sst.Link("xr2r_3_0_5")
xr2r_3_0_5.connect( (s_28_rtr, "xPos", "10000ps"), (s_29_rtr, "xNeg", "10000ps") )
xr2r_3_0_6 = sst.Link("xr2r_3_0_6")
xr2r_3_0_6.connect( (s_29_rtr, "xPos", "10000ps"), (s_30_rtr, "xNeg", "10000ps") )
xr2r_3_0_7 = sst.Link("xr2r_3_0_7")
xr2r_3_0_7.connect( (s_30_rtr, "xPos", "10000ps"), (s_31_rtr, "xNeg", "10000ps") )
xr2r_4_0_0 = sst.Link("xr2r_4_0_0")
xr2r_4_0_0.connect( (s_32_rtr, "xNeg", "10000ps"), (s_39_rtr, "xPos", "10000ps") )
xr2r_4_0_1 = sst.Link("xr2r_4_0_1")
xr2r_4_0_1.connect( (s_32_rtr, "xPos", "10000ps"), (s_33_rtr, "xNeg", "10000ps") )
xr2r_4_0_2 = sst.Link("xr2r_4_0_2")
xr2r_4_0_2.connect( (s_33_rtr, "xPos", "10000ps"), (s_34_rtr, "xNeg", "10000ps") )
xr2r_4_0_3 = sst.Link("xr2r_4_0_3")
xr2r_4_0_3.connect( (s_34_rtr, "xPos", "10000ps"), (s_35_rtr, "xNeg", "10000ps") )
xr2r_4_0_4 = sst.Link("xr2r_4_0_4")
xr2r_4_0_4.connect( (s_35_rtr, "xPos", "10000ps"), (s_36_rtr, "xNeg", "10000ps") )
xr2r_4_0_5 = sst.Link("xr2r_4_0_5")
xr2r_4_0_5.connect( (s_36_rtr, "xPos", "10000ps"), (s_37_rtr, "xNeg", "10000ps") )
xr2r_4_0_6 = sst.Link("xr2r_4_0_6")
xr2r_4_0_6.connect( (s_37_rtr, "xPos", "10000ps"), (s_38_rtr, "xNeg", "10000ps") )
xr2r_4_0_7 = sst.Link("xr2r_4_0_7")
xr2r_4_0_7.connect( (s_38_rtr, "xPos", "10000ps"), (s_39_rtr, "xNeg", "10000ps") )
xr2r_5_0_0 = sst.Link("xr2r_5_0_0")
xr2r_5_0_0.connect( (s_40_rtr, "xNeg", "10000ps"), (s_47_rtr, "xPos", "10000ps") )
xr2r_5_0_1 = sst.Link("xr2r_5_0_1")
xr2r_5_0_1.connect( (s_40_rtr, "xPos", "10000ps"), (s_41_rtr, "xNeg", "10000ps") )
xr2r_5_0_2 = sst.Link("xr2r_5_0_2")
xr2r_5_0_2.connect( (s_41_rtr, "xPos", "10000ps"), (s_42_rtr, "xNeg", "10000ps") )
xr2r_5_0_3 = sst.Link("xr2r_5_0_3")
xr2r_5_0_3.connect( (s_42_rtr, "xPos", "10000ps"), (s_43_rtr, "xNeg", "10000ps") )
xr2r_5_0_4 = sst.Link("xr2r_5_0_4")
xr2r_5_0_4.connect( (s_43_rtr, "xPos", "10000ps"), (s_44_rtr, "xNeg", "10000ps") )
xr2r_5_0_5 = sst.Link("xr2r_5_0_5")
xr2r_5_0_5.connect( (s_44_rtr, "xPos", "10000ps"), (s_45_rtr, "xNeg", "10000ps") )
xr2r_5_0_6 = sst.Link("xr2r_5_0_6")
xr2r_5_0_6.connect( (s_45_rtr, "xPos", "10000ps"), (s_46_rtr, "xNeg", "10000ps") )
xr2r_5_0_7 = sst.Link("xr2r_5_0_7")
xr2r_5_0_7.connect( (s_46_rtr, "xPos", "10000ps"), (s_47_rtr, "xNeg", "10000ps") )
xr2r_6_0_0 = sst.Link("xr2r_6_0_0")
xr2r_6_0_0.connect( (s_48_rtr, "xNeg", "10000ps"), (s_55_rtr, "xPos", "10000ps") )
xr2r_6_0_1 = sst.Link("xr2r_6_0_1")
xr2r_6_0_1.connect( (s_48_rtr, "xPos", "10000ps"), (s_49_rtr, "xNeg", "10000ps") )
xr2r_6_0_2 = sst.Link("xr2r_6_0_2")
xr2r_6_0_2.connect( (s_49_rtr, "xPos", "10000ps"), (s_50_rtr, "xNeg", "10000ps") )
xr2r_6_0_3 = sst.Link("xr2r_6_0_3")
xr2r_6_0_3.connect( (s_50_rtr, "xPos", "10000ps"), (s_51_rtr, "xNeg", "10000ps") )
xr2r_6_0_4 = sst.Link("xr2r_6_0_4")
xr2r_6_0_4.connect( (s_51_rtr, "xPos", "10000ps"), (s_52_rtr, "xNeg", "10000ps") )
xr2r_6_0_5 = sst.Link("xr2r_6_0_5")
xr2r_6_0_5.connect( (s_52_rtr, "xPos", "10000ps"), (s_53_rtr, "xNeg", "10000ps") )
xr2r_6_0_6 = sst.Link("xr2r_6_0_6")
xr2r_6_0_6.connect( (s_53_rtr, "xPos", "10000ps"), (s_54_rtr, "xNeg", "10000ps") )
xr2r_6_0_7 = sst.Link("xr2r_6_0_7")
xr2r_6_0_7.connect( (s_54_rtr, "xPos", "10000ps"), (s_55_rtr, "xNeg", "10000ps") )
xr2r_7_0_0 = sst.Link("xr2r_7_0_0")
xr2r_7_0_0.connect( (s_56_rtr, "xNeg", "10000ps"), (s_63_rtr, "xPos", "10000ps") )
xr2r_7_0_1 = sst.Link("xr2r_7_0_1")
xr2r_7_0_1.connect( (s_56_rtr, "xPos", "10000ps"), (s_57_rtr, "xNeg", "10000ps") )
xr2r_7_0_2 = sst.Link("xr2r_7_0_2")
xr2r_7_0_2.connect( (s_57_rtr, "xPos", "10000ps"), (s_58_rtr, "xNeg", "10000ps") )
xr2r_7_0_3 = sst.Link("xr2r_7_0_3")
xr2r_7_0_3.connect( (s_58_rtr, "xPos", "10000ps"), (s_59_rtr, "xNeg", "10000ps") )
xr2r_7_0_4 = sst.Link("xr2r_7_0_4")
xr2r_7_0_4.connect( (s_59_rtr, "xPos", "10000ps"), (s_60_rtr, "xNeg", "10000ps") )
xr2r_7_0_5 = sst.Link("xr2r_7_0_5")
xr2r_7_0_5.connect( (s_60_rtr, "xPos", "10000ps"), (s_61_rtr, "xNeg", "10000ps") )
xr2r_7_0_6 = sst.Link("xr2r_7_0_6")
xr2r_7_0_6.connect( (s_61_rtr, "xPos", "10000ps"), (s_62_rtr, "xNeg", "10000ps") )
xr2r_7_0_7 = sst.Link("xr2r_7_0_7")
xr2r_7_0_7.connect( (s_62_rtr, "xPos", "10000ps"), (s_63_rtr, "xNeg", "10000ps") )
yr2r_0_0_0 = sst.Link("yr2r_0_0_0")
yr2r_0_0_0.connect( (s_0_rtr, "yNeg", "10000ps"), (s_56_rtr, "yPos", "10000ps") )
yr2r_0_0_1 = sst.Link("yr2r_0_0_1")
yr2r_0_0_1.connect( (s_0_rtr, "yPos", "10000ps"), (s_8_rtr, "yNeg", "10000ps") )
yr2r_0_0_2 = sst.Link("yr2r_0_0_2")
yr2r_0_0_2.connect( (s_8_rtr, "yPos", "10000ps"), (s_16_rtr, "yNeg", "10000ps") )
yr2r_0_0_3 = sst.Link("yr2r_0_0_3")
yr2r_0_0_3.connect( (s_16_rtr, "yPos", "10000ps"), (s_24_rtr, "yNeg", "10000ps") )
yr2r_0_0_4 = sst.Link("yr2r_0_0_4")
yr2r_0_0_4.connect( (s_24_rtr, "yPos", "10000ps"), (s_32_rtr, "yNeg", "10000ps") )
yr2r_0_0_5 = sst.Link("yr2r_0_0_5")
yr2r_0_0_5.connect( (s_32_rtr, "yPos", "10000ps"), (s_40_rtr, "yNeg", "10000ps") )
yr2r_0_0_6 = sst.Link("yr2r_0_0_6")
yr2r_0_0_6.connect( (s_40_rtr, "yPos", "10000ps"), (s_48_rtr, "yNeg", "10000ps") )
yr2r_0_0_7 = sst.Link("yr2r_0_0_7")
yr2r_0_0_7.connect( (s_48_rtr, "yPos", "10000ps"), (s_56_rtr, "yNeg", "10000ps") )
yr2r_1_0_0 = sst.Link("yr2r_1_0_0")
yr2r_1_0_0.connect( (s_1_rtr, "yNeg", "10000ps"), (s_57_rtr, "yPos", "10000ps") )
yr2r_1_0_1 = sst.Link("yr2r_1_0_1")
yr2r_1_0_1.connect( (s_1_rtr, "yPos", "10000ps"), (s_9_rtr, "yNeg", "10000ps") )
yr2r_1_0_2 = sst.Link("yr2r_1_0_2")
yr2r_1_0_2.connect( (s_9_rtr, "yPos", "10000ps"), (s_17_rtr, "yNeg", "10000ps") )
yr2r_1_0_3 = sst.Link("yr2r_1_0_3")
yr2r_1_0_3.connect( (s_17_rtr, "yPos", "10000ps"), (s_25_rtr, "yNeg", "10000ps") )
yr2r_1_0_4 = sst.Link("yr2r_1_0_4")
yr2r_1_0_4.connect( (s_25_rtr, "yPos", "10000ps"), (s_33_rtr, "yNeg", "10000ps") )
yr2r_1_0_5 = sst.Link("yr2r_1_0_5")
yr2r_1_0_5.connect( (s_33_rtr, "yPos", "10000ps"), (s_41_rtr, "yNeg", "10000ps") )
yr2r_1_0_6 = sst.Link("yr2r_1_0_6")
yr2r_1_0_6.connect( (s_41_rtr, "yPos", "10000ps"), (s_49_rtr, "yNeg", "10000ps") )
yr2r_1_0_7 = sst.Link("yr2r_1_0_7")
yr2r_1_0_7.connect( (s_49_rtr, "yPos", "10000ps"), (s_57_rtr, "yNeg", "10000ps") )
yr2r_2_0_0 = sst.Link("yr2r_2_0_0")
yr2r_2_0_0.connect( (s_2_rtr, "yNeg", "10000ps"), (s_58_rtr, "yPos", "10000ps") )
yr2r_2_0_1 = sst.Link("yr2r_2_0_1")
yr2r_2_0_1.connect( (s_2_rtr, "yPos", "10000ps"), (s_10_rtr, "yNeg", "10000ps") )
yr2r_2_0_2 = sst.Link("yr2r_2_0_2")
yr2r_2_0_2.connect( (s_10_rtr, "yPos", "10000ps"), (s_18_rtr, "yNeg", "10000ps") )
yr2r_2_0_3 = sst.Link("yr2r_2_0_3")
yr2r_2_0_3.connect( (s_18_rtr, "yPos", "10000ps"), (s_26_rtr, "yNeg", "10000ps") )
yr2r_2_0_4 = sst.Link("yr2r_2_0_4")
yr2r_2_0_4.connect( (s_26_rtr, "yPos", "10000ps"), (s_34_rtr, "yNeg", "10000ps") )
yr2r_2_0_5 = sst.Link("yr2r_2_0_5")
yr2r_2_0_5.connect( (s_34_rtr, "yPos", "10000ps"), (s_42_rtr, "yNeg", "10000ps") )
yr2r_2_0_6 = sst.Link("yr2r_2_0_6")
yr2r_2_0_6.connect( (s_42_rtr, "yPos", "10000ps"), (s_50_rtr, "yNeg", "10000ps") )
yr2r_2_0_7 = sst.Link("yr2r_2_0_7")
yr2r_2_0_7.connect( (s_50_rtr, "yPos", "10000ps"), (s_58_rtr, "yNeg", "10000ps") )
yr2r_3_0_0 = sst.Link("yr2r_3_0_0")
yr2r_3_0_0.connect( (s_3_rtr, "yNeg", "10000ps"), (s_59_rtr, "yPos", "10000ps") )
yr2r_3_0_1 = sst.Link("yr2r_3_0_1")
yr2r_3_0_1.connect( (s_3_rtr, "yPos", "10000ps"), (s_11_rtr, "yNeg", "10000ps") )
yr2r_3_0_2 = sst.Link("yr2r_3_0_2")
yr2r_3_0_2.connect( (s_11_rtr, "yPos", "10000ps"), (s_19_rtr, "yNeg", "10000ps") )
yr2r_3_0_3 = sst.Link("yr2r_3_0_3")
yr2r_3_0_3.connect( (s_19_rtr, "yPos", "10000ps"), (s_27_rtr, "yNeg", "10000ps") )
yr2r_3_0_4 = sst.Link("yr2r_3_0_4")
yr2r_3_0_4.connect( (s_27_rtr, "yPos", "10000ps"), (s_35_rtr, "yNeg", "10000ps") )
yr2r_3_0_5 = sst.Link("yr2r_3_0_5")
yr2r_3_0_5.connect( (s_35_rtr, "yPos", "10000ps"), (s_43_rtr, "yNeg", "10000ps") )
yr2r_3_0_6 = sst.Link("yr2r_3_0_6")
yr2r_3_0_6.connect( (s_43_rtr, "yPos", "10000ps"), (s_51_rtr, "yNeg", "10000ps") )
yr2r_3_0_7 = sst.Link("yr2r_3_0_7")
yr2r_3_0_7.connect( (s_51_rtr, "yPos", "10000ps"), (s_59_rtr, "yNeg", "10000ps") )
yr2r_4_0_0 = sst.Link("yr2r_4_0_0")
yr2r_4_0_0.connect( (s_4_rtr, "yNeg", "10000ps"), (s_60_rtr, "yPos", "10000ps") )
yr2r_4_0_1 = sst.Link("yr2r_4_0_1")
yr2r_4_0_1.connect( (s_4_rtr, "yPos", "10000ps"), (s_12_rtr, "yNeg", "10000ps") )
yr2r_4_0_2 = sst.Link("yr2r_4_0_2")
yr2r_4_0_2.connect( (s_12_rtr, "yPos", "10000ps"), (s_20_rtr, "yNeg", "10000ps") )
yr2r_4_0_3 = sst.Link("yr2r_4_0_3")
yr2r_4_0_3.connect( (s_20_rtr, "yPos", "10000ps"), (s_28_rtr, "yNeg", "10000ps") )
yr2r_4_0_4 = sst.Link("yr2r_4_0_4")
yr2r_4_0_4.connect( (s_28_rtr, "yPos", "10000ps"), (s_36_rtr, "yNeg", "10000ps") )
yr2r_4_0_5 = sst.Link("yr2r_4_0_5")
yr2r_4_0_5.connect( (s_36_rtr, "yPos", "10000ps"), (s_44_rtr, "yNeg", "10000ps") )
yr2r_4_0_6 = sst.Link("yr2r_4_0_6")
yr2r_4_0_6.connect( (s_44_rtr, "yPos", "10000ps"), (s_52_rtr, "yNeg", "10000ps") )
yr2r_4_0_7 = sst.Link("yr2r_4_0_7")
yr2r_4_0_7.connect( (s_52_rtr, "yPos", "10000ps"), (s_60_rtr, "yNeg", "10000ps") )
yr2r_5_0_0 = sst.Link("yr2r_5_0_0")
yr2r_5_0_0.connect( (s_5_rtr, "yNeg", "10000ps"), (s_61_rtr, "yPos", "10000ps") )
yr2r_5_0_1 = sst.Link("yr2r_5_0_1")
yr2r_5_0_1.connect( (s_5_rtr, "yPos", "10000ps"), (s_13_rtr, "yNeg", "10000ps") )
yr2r_5_0_2 = sst.Link("yr2r_5_0_2")
yr2r_5_0_2.connect( (s_13_rtr, "yPos", "10000ps"), (s_21_rtr, "yNeg", "10000ps") )
yr2r_5_0_3 = sst.Link("yr2r_5_0_3")
yr2r_5_0_3.connect( (s_21_rtr, "yPos", "10000ps"), (s_29_rtr, "yNeg", "10000ps") )
yr2r_5_0_4 = sst.Link("yr2r_5_0_4")
yr2r_5_0_4.connect( (s_29_rtr, "yPos", "10000ps"), (s_37_rtr, "yNeg", "10000ps") )
yr2r_5_0_5 = sst.Link("yr2r_5_0_5")
yr2r_5_0_5.connect( (s_37_rtr, "yPos", "10000ps"), (s_45_rtr, "yNeg", "10000ps") )
yr2r_5_0_6 = sst.Link("yr2r_5_0_6")
yr2r_5_0_6.connect( (s_45_rtr, "yPos", "10000ps"), (s_53_rtr, "yNeg", "10000ps") )
yr2r_5_0_7 = sst.Link("yr2r_5_0_7")
yr2r_5_0_7.connect( (s_53_rtr, "yPos", "10000ps"), (s_61_rtr, "yNeg", "10000ps") )
yr2r_6_0_0 = sst.Link("yr2r_6_0_0")
yr2r_6_0_0.connect( (s_6_rtr, "yNeg", "10000ps"), (s_62_rtr, "yPos", "10000ps") )
yr2r_6_0_1 = sst.Link("yr2r_6_0_1")
yr2r_6_0_1.connect( (s_6_rtr, "yPos", "10000ps"), (s_14_rtr, "yNeg", "10000ps") )
yr2r_6_0_2 = sst.Link("yr2r_6_0_2")
yr2r_6_0_2.connect( (s_14_rtr, "yPos", "10000ps"), (s_22_rtr, "yNeg", "10000ps") )
yr2r_6_0_3 = sst.Link("yr2r_6_0_3")
yr2r_6_0_3.connect( (s_22_rtr, "yPos", "10000ps"), (s_30_rtr, "yNeg", "10000ps") )
yr2r_6_0_4 = sst.Link("yr2r_6_0_4")
yr2r_6_0_4.connect( (s_30_rtr, "yPos", "10000ps"), (s_38_rtr, "yNeg", "10000ps") )
yr2r_6_0_5 = sst.Link("yr2r_6_0_5")
yr2r_6_0_5.connect( (s_38_rtr, "yPos", "10000ps"), (s_46_rtr, "yNeg", "10000ps") )
yr2r_6_0_6 = sst.Link("yr2r_6_0_6")
yr2r_6_0_6.connect( (s_46_rtr, "yPos", "10000ps"), (s_54_rtr, "yNeg", "10000ps") )
yr2r_6_0_7 = sst.Link("yr2r_6_0_7")
yr2r_6_0_7.connect( (s_54_rtr, "yPos", "10000ps"), (s_62_rtr, "yNeg", "10000ps") )
yr2r_7_0_0 = sst.Link("yr2r_7_0_0")
yr2r_7_0_0.connect( (s_7_rtr, "yNeg", "10000ps"), (s_63_rtr, "yPos", "10000ps") )
yr2r_7_0_1 = sst.Link("yr2r_7_0_1")
yr2r_7_0_1.connect( (s_7_rtr, "yPos", "10000ps"), (s_15_rtr, "yNeg", "10000ps") )
yr2r_7_0_2 = sst.Link("yr2r_7_0_2")
yr2r_7_0_2.connect( (s_15_rtr, "yPos", "10000ps"), (s_23_rtr, "yNeg", "10000ps") )
yr2r_7_0_3 = sst.Link("yr2r_7_0_3")
yr2r_7_0_3.connect( (s_23_rtr, "yPos", "10000ps"), (s_31_rtr, "yNeg", "10000ps") )
yr2r_7_0_4 = sst.Link("yr2r_7_0_4")
yr2r_7_0_4.connect( (s_31_rtr, "yPos", "10000ps"), (s_39_rtr, "yNeg", "10000ps") )
yr2r_7_0_5 = sst.Link("yr2r_7_0_5")
yr2r_7_0_5.connect( (s_39_rtr, "yPos", "10000ps"), (s_47_rtr, "yNeg", "10000ps") )
yr2r_7_0_6 = sst.Link("yr2r_7_0_6")
yr2r_7_0_6.connect( (s_47_rtr, "yPos", "10000ps"), (s_55_rtr, "yNeg", "10000ps") )
yr2r_7_0_7 = sst.Link("yr2r_7_0_7")
yr2r_7_0_7.connect( (s_55_rtr, "yPos", "10000ps"), (s_63_rtr, "yNeg", "10000ps") )
# End of generated output.
