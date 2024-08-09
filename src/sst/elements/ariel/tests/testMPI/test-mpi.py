import sst
import sys
import os
import argparse
import pathlib

parser = argparse.ArgumentParser(
    prog=f'sst [sst-args] test-mpi.py --',
    description='Used for testing Ariel\'s MPI features')

parser.add_argument('program', help='Which program to run. Either "hello" or "reduce".')
parser.add_argument('-r', dest='ranks', default=1, help='How many ranks of the traced program to run.')
parser.add_argument('-a', dest='tracerank', default=0, help='Which of the MPI ranks will be traced.')
parser.add_argument('-t', dest='threads', default=1, help='The number of OpenMP threads to use per rank.')
parser.add_argument('-s', dest='size', default=2048, help='The input value for the "reduce" program')
parser.add_argument('-o', dest='output', help='Optional argument to both programs to change stdout')

args = parser.parse_args()

ncores    = int(args.threads)
mpiranks  = int(args.ranks)
tracerank = int(args.tracerank)
size      = int(args.size)

if args.program not in ['hello', 'reduce']:
    print('Error: supported values for `program` are "hello" and "reduce".')

program_string = f'./{args.program}'
if args.program == 'reduce':
    program_string += f' (size={size})'

print(f'mpi-test.py: Running {program_string} with {mpiranks} rank(s) and {ncores} thread(s) per rank. Tracing rank {tracerank}')

os.environ['OMP_NUM_THREADS'] = str(ncores)


#########################################################################
## Declare components
#########################################################################
core    = sst.Component("core", "ariel.ariel")
cache   = [sst.Component("cache_"+str(i), "memHierarchy.Cache") for i in range(ncores)]
memctrl = sst.Component("memory", "memHierarchy.MemController")
bus     = sst.Component("bus", "memHierarchy.Bus")

#########################################################################
## Set component parameters and fill subcomponent slots
#########################################################################
exe = f'./{args.program}'

# 2.4GHz cores. One for each omp thread
core.addParams({
    "clock"        : "2.4GHz",
    "verbose"      : 1,
    "executable"   : exe,
    "arielmode"    : 0, # Disable tracing at start
    "corecount"    : ncores,
    "mpimode"      : 1,
    "mpiranks"     : mpiranks,
    "mpitracerank" : tracerank,
})

# This should be detected in Ariel but checking here allows us to fail
# the test faster
if not pathlib.Path(exe).exists():
    print(f'test-mpi.py: Error: executable {exe} does not exist')
    sys.exit(1)

# Set the size of the reduce vector and optionally set the output file
if args.program == "reduce":
    if args.output is not None:
        core.addParams({
            "appargcount" : 2,
            "apparg0" : size,
            "apparg1" : args.output,
        })
    else:
        core.addParams({
            "appargcount" : 1,
            "apparg0" : size,
        })
# Set the output file for the hello program
elif args.output is not None:
    core.addParams({
        "appargcount" : 1,
        "apparg0" : args.output,
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
cache_bus  = [sst.Link("cache_" + str(i) + "_to_bus") for i in range(ncores)]
bus_mem    = sst.Link("bus_to_memory")

#########################################################################
## Connect components with the links
#########################################################################
[core_cache[i].connect( (core, "cache_link_"+str(i), "100ps"), (cache[i], "high_network_0", "100ps") ) for i in range(ncores)]
[cache_bus[i].connect( (cache[i], "low_network_0", "100ps"), (bus, "high_network_"+str(i), "100ps") ) for i in range(ncores)]
bus_mem.connect( (bus, "low_network_0", "100ps"), (memctrl, "direct_link", "100ps") )

#########################################################################
## Define SST core options
#########################################################################
sst.setProgramOption("stop-at", "200ms")
sst.setStatisticOutput("sst.statoutputtxt")
sst.setStatisticOutputOptions( { "filepath"  : "stats.csv" })
sst.setStatisticLoadLevel(5)
sst.enableAllStatisticsForAllComponents()
