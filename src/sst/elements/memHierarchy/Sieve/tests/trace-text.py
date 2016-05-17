import sst
import os
from optparse import OptionParser

# Define SST core options
sst.setProgramOption("timebase", "1ps")

sieveProfilerParams = {
        "profiler": "cassini.AddrHistogrammer",
        "profiler.addr_cutoff" : "16GiB"
        }

# Define the simulation components
comp_cpu = sst.Component("cpu", "prospero.prosperoCPU")
comp_cpu.addParams({
      	"verbose" : "0",
	"reader" : "prospero.ProsperoTextTraceReader",
	"readerParams.file" : os.getcwd()+"/sieveprospero-0.trace"
})

comp_sieve = sst.Component("sieve", "memHierarchy.Sieve")
comp_sieve.addParams({
      "cache_size": "2 KB",
      "associativity": 8,
      "cache_line_size": 64
})

comp_sieve.addParams(sieveProfilerParams)

link_cpu_sieve_link = sst.Link("link_cpu_sieve_link")
link_cpu_sieve_link.connect( (comp_cpu, "cache_link", "1000ps"), (comp_sieve, "cpu_link_0", "1000ps") )

statoutputs = dict([(1,"sst.statOutputConsole"), (2,"sst.statOutputCSV"), (3,"sst.statOutputTXT")])

sst.setStatisticLoadLevel(7)
sst.setStatisticOutput(statoutputs[2])
sst.enableStatisticForComponentType("memHierarchy.Sieve",
                                    "histogram_reads",
                                        {"type":"sst.HistogramStatistic",
                                         "minvalue" : "0",
                                         "numbins"  : "6", 
                                         "binwidth" : "4096",
                                         "addr_cutoff" : "16GiB"
                                         #"rate" : "100ns"
                                         })
sst.enableStatisticForComponentType("memHierarchy.Sieve",
                                    "histogram_writes",
                                        {"type":"sst.HistogramStatistic",
                                         "minvalue" : "0",
                                         "numbins"  : "6", 
                                         "binwidth" : "4096",
                                         "addr_cutoff" : "16GiB"
                                         })
