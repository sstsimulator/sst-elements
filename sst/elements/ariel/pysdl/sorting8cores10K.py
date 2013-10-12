import sst

def sstcreatemodel():

    sst.creategraph()

    if sst.verbose():
        print "Verbose Model"

    id = sst.createcomponent("a0", "ariel.ariel")
    sst.addcompparam(id, "verbose", "0")
    sst.addcompparam(id, "executable", "/home/sdhammo/xgc/xgc_co_design/sorting/spsort/bsr")
    sst.addcompparam(id, "arieltool", "/home/sdhammo/subversion/sst-simulator/sst/elements/ariel/tool/arieltool.so")
    sst.addcompparam(id, "fastmempagecount", "131072")

    sst.addcompparam(id, "appargcount", "6")
    sst.addcompparam(id, "apparg0", "10000")
    sst.addcompparam(id, "apparg1", "1")
    sst.addcompparam(id, "apparg2", "1048576")
    sst.addcompparam(id, "apparg3", "512")
    sst.addcompparam(id, "apparg4", "4")
    sst.addcompparam(id, "apparg5", "8")

    corecount = 8;

    sst.addcompparam(id, "corecount", str(corecount))

    print "Simulation core count: ", corecount

    membus = sst.createcomponent("membus", "memHierarchy.Bus")
    sst.addcompparam(membus, "numPorts", str(corecount + 4))
    sst.addcompparam(membus, "busDelay", "1ns")

    # Create a series of caches all linked to the Ariel "core"
    max_core = corecount - 1;

    for x in range(0, 4):
       sst.addcomplink(id, "cpu_cache_link_" + str(x), "cache_link_" + str(x), "1ns")
       l1id = sst.createcomponent("l1cache_" + str(x), "memHierarchy.Cache")
       sst.addcompparam(l1id, "num_ways", "8")
       sst.addcompparam(l1id, "num_rows", "128")
       sst.addcompparam(l1id, "blocksize", "64")
       sst.addcompparam(l1id, "access_time", "1ns")
       sst.addcompparam(l1id, "num_upstream", "1")
       sst.addcompparam(l1id, "next_level", "l2cache_0")
       sst.addcomplink(l1id, "cpu_cache_link_" + str(x), "upstream0", "1ns")
       sst.addcomplink(l1id, "cache_bus_link_" + str(x), "snoop_link", "1ns")
       sst.addcompparam(l1id, "printStats", "1")
       sst.addcomplink(membus, "cache_bus_link_" + str(x), "port" + str(x), "1ns")
       print "added cache_bus_link_%d at port %d\n"%(x,x)

    for x in range(4, 8):
       sst.addcomplink(id, "cpu_cache_link_" + str(x), "cache_link_" + str(x), "1ns")
       l1id = sst.createcomponent("l1cache_" + str(x), "memHierarchy.Cache")
       sst.addcompparam(l1id, "num_ways", "8")
       sst.addcompparam(l1id, "num_rows", "128")
       sst.addcompparam(l1id, "blocksize", "64")
       sst.addcompparam(l1id, "access_time", "1ns")
       sst.addcompparam(l1id, "num_upstream", "1")
       sst.addcompparam(l1id, "next_level", "l2cache_1")
       sst.addcomplink(l1id, "cpu_cache_link_" + str(x), "upstream0", "1ns")
       sst.addcomplink(l1id, "cache_bus_link_" + str(x), "snoop_link", "1ns")
       sst.addcompparam(l1id, "printStats", "1")
       sst.addcomplink(membus, "cache_bus_link_" + str(x), "port" + str(x), "1ns")
       print "added cache_bus_link_%d at port %d\n"%(x,x)

    l2_q1 = sst.createcomponent("l2cache_0", "memHierarchy.Cache")
    sst.addcompparam(l2_q1, "num_ways", "8")
    sst.addcompparam(l2_q1, "num_rows", "512")
    sst.addcompparam(l2_q1, "blocksize", "64")
    sst.addcompparam(l2_q1, "access_time", "6ns")
    sst.addcompparam(l2_q1, "printStats", "1")
    sst.addcomplink(l2_q1, "l2cache_0_link", "snoop_link", "1ns")

    l2_q2 = sst.createcomponent("l2cache_1", "memHierarchy.Cache")
    sst.addcompparam(l2_q2, "num_ways", "8")
    sst.addcompparam(l2_q2, "num_rows", "512")
    sst.addcompparam(l2_q2, "blocksize", "64")
    sst.addcompparam(l2_q2, "access_time", "6ns")
    sst.addcompparam(l2_q2, "printStats", "1")
    sst.addcomplink(l2_q2, "l2cache_1_link", "snoop_link", "1ns")

    # Put a link to memory from the memory bus
    sst.addcomplink(membus, "mem_bus_link", "port%d"%(corecount), "1ns")
    sst.addcomplink(membus, "memNear_bus_link", "port%d"%(corecount+1), "1ns")
    sst.addcomplink(membus, "l2cache_0_link", "port%d"%(corecount+2), "1ns")
    sst.addcomplink(membus, "l2cache_1_link", "port%d"%(corecount+3), "1ns")

    memory = sst.createcomponent("memory", "memHierarchy.MemController")
    sst.addcompparam(memory, "access_time", "70ns")
    sst.addcompparam(memory, "rangeStart", "0x20000000")
    sst.addcompparam(memory, "mem_size", "512")
    sst.addcompparam(memory, "clock", "1GHz")
    sst.addcompparam(memory, "printStats", "1")
    sst.addcomplink(memory, "mem_bus_link", "snoop_link", "50ps")

    nearMemory = sst.createcomponent("nearmemory", "memHierarchy.MemController")
    sst.addcompparam(nearMemory, "access_time", "50ns")
    sst.addcompparam(nearMemory, "rangeStart", "0x0")
    sst.addcompparam(nearMemory, "clock", "1GHz")
    sst.addcompparam(nearMemory, "mem_size", "512")
    sst.addcompparam(nearMemory, "printStats", "1")
    sst.addcomplink(nearMemory, "memNear_bus_link", "snoop_link", "50ps")

    print "done configuring SST"

    return 0
