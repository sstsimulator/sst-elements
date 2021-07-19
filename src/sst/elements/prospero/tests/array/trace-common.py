# Automatically generated SST Python input
import sst
import os
import sys,getopt

Tracetype = "Type Error"
traceFile = "File Error" 
memSize = "4096"
useDramSim="no"

def main():
    global Tracetype
    global traceFile
    global memSize
    global useDramSim
 

    try:
        opts, args = getopt.getopt(sys.argv[1:], "", ["TraceType=","UseDramSim="])
    except getopt.GetopError as err:
        print(str(err))
        sys.exit(2)
    for o, a in opts:
        # print "args are ", o, "and", a

        if o in ("--TraceType"):
            if a == "text":
                Tracetype = "Text"
                traceFile = "sstprospero-0-0.trace"
            elif a == "binary":
                Tracetype = "Binary"
                traceFile = "sstprospero-0-0-bin.trace"
            elif a == "compressed":
                # print "args are ", o, "and", a
                Tracetype = "CompressedBinary"
                traceFile = "sstprospero-0-0-gz.trace"
            else:
                print("no match a= ", a)
                print("Found nothing for o", o)
        elif o in ("--UseDramSim"):
            if a == "yes":
                useDramSim = 'yes'
                memSize = "512"
        else:
            print("no match for o", o)
            assert False, "Unknown Options !"

main()

# Define SST core options
sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stopAtCycle", "5s")

# Define the simulation components
comp_cpu = sst.Component("cpu", "prospero.prosperoCPU")
comp_cpu.addParams({
       "verbose" : "0",
       "reader" : "prospero.Prospero" + Tracetype + "TraceReader",
       "readerParams.file" : traceFile
})
comp_l1cache = sst.Component("l1cache", "memHierarchy.Cache")
comp_l1cache.addParams({
      "access_latency_cycles" : "1",
      "cache_frequency" : "2 Ghz",
      "replacement_policy" : "lru",
      "coherence_protocol" : "MESI",
      "associativity" : "8",
      "cache_line_size" : "64",
      "L1" : "1",
      "cache_size" : "64 KB"
})
comp_memctrl = sst.Component("memory", "memHierarchy.MemController")
comp_memctrl.addParams({
      "coherence_protocol" : "MESI",
      "clock" : "1GHz"
})
if useDramSim == "yes":
    memory = comp_memctrl.setSubComponent("backend", "memHierarchy.dramsim")
    memory.addParams({
        "system_ini" : "system.ini",
        "device_ini" : "DDR3_micron_32M_8B_x4_sg125.ini",
        "mem_size" : str(memSize) + "MiB",
    })
else:
    memory = comp_memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
    memory.addParams({
        "access_time" : "1000 ns",
        "mem_size" : str(memSize) + "MiB",
    })
    
# Define the simulation links
link_cpu_cache_link = sst.Link("link_cpu_cache_link")
link_cpu_cache_link.connect( (comp_cpu, "cache_link", "1000ps"), (comp_l1cache, "high_network_0", "1000ps") )
link_mem_bus_link = sst.Link("link_mem_bus_link")
link_mem_bus_link.connect( (comp_l1cache, "low_network_0", "50ps"), (comp_memctrl, "direct_link", "50ps") )
# End of generated output.
