# Automatically generated SST Python input
import sst
import os
import sys,getopt

Tracetype = "Type Error"
traceFile = "File Error"
traceDir = "Dir Error"
memSize = "4096"
useTimingDram="no"

def main():
    global Tracetype
    global traceFile
    global traceDir
    global memSize
    global useTimingDram

    try:
        opts, args = getopt.getopt(sys.argv[1:], "", ["TraceType=","UseTimingDram=","TraceDir="])
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
        elif o in ("--UseTimingDram"):
            if a == "yes":
                useTimingDram = 'yes'
        elif o in ("--TraceDir"):
            traceDir=a
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
       "readerParams.file" : traceDir + "/" + traceFile
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
      "clock" : "1GHz",
      "addr_range_start" : 0,
})
if useTimingDram == "yes":
    memory = comp_memctrl.setSubComponent("backend", "memHierarchy.timingDRAM")
    memory.addParams({
        "id" : 0,
        "addrMapper" : "memHierarchy.roundRobinAddrMapper",
        "addrMapper.interleave_size" : "64B",
        "addrMapper.row_size" : "1KiB",
        "clock" : "800MHz",
        "mem_size" : str(memSize) + "MiB",
        "channels" : 2,
        "channel.numRanks" : 2,
        "channel.rank.numBanks" : 2,
        "channel.transaction_Q_size" : 32,
        "channel.rank.bank.CL" : 14,
        "channel.rank.bank.CL_WR" : 12,
        "channel.rank.bank.RCD" : 14,
        "channel.rank.bank.TRP" : 14,
        "channel.rank.bank.dataCycles" : 2,
        "channel.rank.bank.pagePolicy" : "memHierarchy.simplePagePolicy",
        "channel.rank.bank.transactionQ" : "memHierarchy.reorderTransactionQ",
        "channel.rank.bank.pagePolicy.close" : 0,
        "printconfig" : 0,
        "channel.printconfig" : 0,
        "channel.rank.printconfig" : 0,
        "channel.rank.bank.printconfig" : 0,
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
