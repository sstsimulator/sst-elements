import sst
import os

#set the number of threads
os.environ['OMP_NUM_THREADS']="16"
corecount = 16

memDebug = 0
memDebugLevel = 7
baseclock = 2660  # in MHz
clock = "%g MHz"%(baseclock)
busLat = "50ps"

memSize = 16384 # in MB"
pageSize = 1  # in KB"
num_pages = memSize * 1024 / pageSize

appArgs = ({
        "executable": "./ompsievetest",
    })

# ariel cpu
ariel = sst.Component("a0", "ariel.ariel")
ariel.addParams(appArgs)
ariel.addParams({
    "verbose" : 1,
    "alloctracker" : 1,
    "clock" : clock,
    "maxcorequeue" : 256,
    "maxissuepercycle" : 2,
    "pipetimeout" : 0,
    "corecount" : corecount,
    "arielmode" : 1,
    "arielstack" : 1,
    "arielinterceptcalls" : 1
})

memmgr = ariel.setSubComponent("memmgr", "memHierarchy.MemoryManagerSieve")
submemmgr = memmgr.setSubComponent("memmgr", "ariel.MemoryManagerSimple")
submemmgr.addParams({
    "pagecount0" : num_pages,
    "pagesize0" : num_pages * 1024
})

sieveId = sst.Component("sieve", "memHierarchy.Sieve")
sieveId.addParams({
    "cache_size": "8MB",
    "associativity": 16,
    "cache_line_size": 64,
    "output_file" : "mallocRank.txt"
})    

for x in range(corecount):
    arielL1Link = sst.Link("cpu_cache_link_%d"%x)
    arielL1Link.connect((ariel, "cache_link_%d"%x, busLat), (sieveId, "cpu_link_%d"%x, busLat))
    alloclink = sst.Link("alloc_link_%d"%x)
    alloclink.connect((memmgr, "alloc_link_%d"%x, busLat), (sieveId, "alloc_link_%d"%x, busLat))

statoutputs = dict([(1,"sst.statOutputConsole"), (2,"sst.statOutputCSV"), (3,"sst.statOutputTXT")])

sst.setStatisticLoadLevel(7)
sst.setStatisticOutput(statoutputs[2])
sst.enableAllStatisticsForAllComponents()

print "done configuring SST"

