import sst

def sstcreatemodel():

    sst.creategraph()

    if sst.verbose():
        print "Verbose Model"

    id = sst.createcomponent("a0", "ariel.ariel")
    sst.addcompparam(id, "verbose", "1")
    sst.addcompparam(id, "maxcorequeue", "1024")
    sst.addcompparam(id, "pipetimeout", "0")
    sst.addcompparam(id, "maxissuepercycle", "4")
    sst.addcompparam(id, "maxtranscore", "32")
    sst.addcompparam(id, "clock", "3.5GHz")
    sst.addcompparam(id, "executable", "/home/sdhammo/exmatex/cachestudy/lulesh/2013-10-31/lulesh2.0")
    sst.addcompparam(id, "arieltool", "/home/sdhammo/subversion/sst-simulator/sst/elements/ariel/tool/arieltool.so")

    corecount = 8;

    sst.addcompparam(id, "corecount", str(corecount))
    sst.addcompparam(id, "appargcount", "4")
    sst.addcompparam(id, "apparg0", "-i")
    sst.addcompparam(id, "apparg1", "2")
    sst.addcompparam(id, "apparg2", "-s")
    sst.addcompparam(id, "apparg3", "50")

    membus = sst.createcomponent("membus", "memHierarchy.Bus")
    sst.addcompparam(membus, "numPorts", str(corecount + corecount + 2))
    sst.addcompparam(membus, "busDelay", "50ps")
    sst.addcompparam(membus, "atomicDelivery", "1")

    # Create a series of caches all linked to the Ariel "core"
    max_core = corecount - 1;

    for x in range(0, corecount):
       sst.addcomplink(id, "cpu_cache_link_" + str(x), "cache_link_" + str(x), "50ps")
       l1id = sst.createcomponent("l1cache_" + str(x), "memHierarchy.Cache")
       sst.addcompparam(l1id, "num_ways", "8")
       sst.addcompparam(l1id, "num_rows", "256")
       sst.addcompparam(l1id, "blocksize", "64")
       sst.addcompparam(l1id, "access_time", "1ns")
       sst.addcompparam(l1id, "num_upstream", "1")
       sst.addcomplink(l1id, "cpu_cache_link_" + str(x), "upstream0", "1ns")
       sst.addcomplink(l1id, "cache_bus_link_" + str(x), "snoop_link", "50ps")
       sst.addcompparam(l1id, "printStats", "1")
       sst.addcompparam(l1id, "next_level", "l2cache_" + str(x))
       sst.addcomplink(membus, "cache_bus_link_" + str(x), "port" + str(x), "50ps")
       print "added cache_bus_link_%d at port %d\n"%(x,x)

    for x in range(0, corecount):
       l2id = sst.createcomponent("l2cache_" + str(x), "memHierarchy.Cache")
       sst.addcomplink(l2id, "l2cache_" + str(x) + "_link", "snoop_link", "50ps")
       sst.addcompparam(l2id, "num_ways", "8")
       sst.addcompparam(l2id, "num_rows", "256")
       sst.addcompparam(l2id, "blocksize", "64")
       sst.addcompparam(l2id, "access_time", "5ns")
       sst.addcompparam(l2id, "printStats", "1")
       sst.addcompparam(l2id, "next_level", "l3cache")
       sst.addcomplink(membus, "l2cache_" + str(x) + "_link", "port" + str(corecount + x), "50ps")

    l3cache = sst.createcomponent("l3cache", "memHierarchy.Cache")
    sst.addcompparam(l3cache, "num_ways", "16")
    sst.addcompparam(l3cache, "num_rows", "8192")
    sst.addcompparam(l3cache, "blocksize", "64")
    sst.addcompparam(l3cache, "access_time", "10ns")
    sst.addcomplink(l3cache, "l3cache_bus_link", "snoop_link", "50ps")
    sst.addcompparam(l3cache, "printStats", "1")

    # Put a link to memory from the memory bus
    sst.addcomplink(membus, "mem_bus_link", "port" + str(corecount + corecount), "50ps")
    sst.addcomplink(membus, "l3cache_bus_link", "port" + str(corecount + corecount + 1), "50ps")

    memory = sst.createcomponent("memory", "memHierarchy.MemController")
    sst.addcompparam(memory, "access_time", "40ns")
    sst.addcompparam(memory, "mem_size", "1024")
    sst.addcompparam(memory, "clock", "1800MHz")
    sst.addcompparam(memory, "use_dramsim", "1")
    sst.addcompparam(memory, "device_ini", "DDR3_micron_32M_8B_x4_sg125.ini")
    sst.addcompparam(memory, "system_ini", "system.ini")
    sst.addcompparam(memory, "printStats", "1")
    sst.addcomplink(memory, "mem_bus_link", "snoop_link", "50ps")

    return 0
