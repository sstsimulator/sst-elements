import sst
import sys
import os

# Detect if we will use MPI mode or not
mpi_mode = True
ncores= 1
mpiranks = 1
tracerank = 0
size = 1024
#size = 2048000

if (len(sys.argv) > 1):
    mpiranks = int(sys.argv[1])
if (len(sys.argv) > 2):
    ncores = int(sys.argv[2])
if (len(sys.argv) > 3):
    size = int(sys.argv[3])

print(f'Running with {mpiranks} ranks and {ncores} threads per rank. Tracing rank {tracerank}. Size {size}')

os.environ['OMP_NUM_THREADS'] = str(ncores)

#########################################################################
## Define SST core options
#########################################################################
# If this demo gets to 100ms, something has gone very wrong!
sst.setProgramOption("stop-at", "200ms")

#########################################################################
## Declare components
#########################################################################
core = sst.Component("core", "ariel.ariel")
cache = [sst.Component("cache_"+str(i), "memHierarchy.Cache") for i in range(ncores)]
memctrl = sst.Component("memory", "memHierarchy.MemController")
bus = sst.Component("bus", "memHierarchy.Bus")

#########################################################################
## Set component parameters and fill subcomponent slots
#########################################################################
# Core: 2.4GHz, 2 accesses/cycle, STREAM (triad) pattern generator with 1000 elements per array
core.addParams({
    "clock" : "2.4GHz",
    "verbose" : 1,
    #"executable" : "./hello-nompi"
    "executable" : "./reduce",
    #"executable" : "/home/prlavin/projects/reference-paper-2024/apps/install/bin/amg",
    "arielmode" : 0,
    "corecount" : ncores,
    "appargcount" : 1,
    #"apparg0" : 500000000,
    #"apparg0" : 250000000,
    "apparg0" : size,
})

if mpi_mode:
    core.addParams({
        "mpilauncher": "./mpilauncher",
        "mpiranks": mpiranks,
        "mpitracerank" : tracerank,
    })

# Cache: L1, 2.4GHz, 2KB, 4-way set associative, 64B lines, LRU replacement, MESI coherence
for i in range(ncores):
    cache[i].addParams({
        "L1" : 1,
        "cache_frequency" : "2.4GHz",
        "access_latency_cycles" : 2,
        "cache_size" : "2KiB",
        "associativity" : 4,
        "replacement_policy" : "lru",
        "coherence_policy" : "MESI",
        "cache_line_size" : 64,
    })

# Memory: 50ns access, 1GB
memctrl.addParams({
    "clock" : "1GHz",
    "backing" : "none", # We're not using real memory values, just addresses
    "addr_range_end" : 1024*1024*1024-1,
})
memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
    "mem_size" : "1GiB",
    "access_time" : "50ns",
})

bus.addParams({
    "bus_frequency": "2.0GHz",
})

#########################################################################
## Declare links
#########################################################################
core_cache = [sst.Link("core_to_cache_"+str(i)) for i in range(ncores)]
cache_bus = [sst.Link("cache_" + str(i) + "_to_bus") for i in range(ncores)]
bus_mem = sst.Link("bus_to_memory")


#########################################################################
## Connect components with the links
#########################################################################
[core_cache[i].connect( (core, "cache_link_"+str(i), "100ps"), (cache[i], "high_network_0", "100ps") ) for i in range(ncores)]
[cache_bus[i].connect( (cache[i], "low_network_0", "100ps"), (bus, "high_network_"+str(i), "100ps") ) for i in range(ncores)]
bus_mem.connect( (bus, "low_network_0", "100ps"), (memctrl, "direct_link", "100ps") )

sst.setStatisticOutput("sst.statoutputtxt")

# Send the statistics to a fiel called 'stats.csv'
sst.setStatisticOutputOptions( { "filepath"  : "stats.csv" })

# Print statistics of level 5 and below (0-5)
sst.setStatisticLoadLevel(5)

# Enable statistics for all the component
sst.enableAllStatisticsForAllComponents()
################################ The End ################################