import sst

def sstcreatemodel():

    sst.creategraph()

    if sst.verbose():
        print "Verbose Model"

    id = sst.createcomponent("a0", "ariel.ariel")
    sst.addcompparam(id, "verbose", "16")
    sst.addcompparam(id, "executable", "/home/sdhammo/subversion/sst-simulator/test/testSuites/testM5/ompatomic/ompatomic.x")
    sst.addcompparam(id, "arieltool", "/home/sdhammo/subversion/sst-simulator/sst/elements/ariel/tool/arieltool.so")

    corecount = 4;

    sst.addcompparam(id, "corecount", str(corecount))

    membus = sst.createcomponent("membus", "memHierarchy.Bus")
    sst.addcompparam(membus, "numPorts", str(corecount + 1))
    sst.addcompparam(membus, "busDelay", "1ns")

    # Create a series of caches all linked to the Ariel "core"
    max_core = corecount - 1;

    for x in range(0, corecount):
       sst.addcomplink(id, "cpu_cache_link_" + str(x), "cache_link_" + str(x), "1ns")
       l1id = sst.createcomponent("l1cache_" + str(x), "memHierarchy.Cache")
       sst.addcompparam(l1id, "num_ways", "8")
       sst.addcompparam(l1id, "num_rows", "128")
       sst.addcompparam(l1id, "blocksize", "64")
       sst.addcompparam(l1id, "access_time", "1ns")
       sst.addcompparam(l1id, "num_upstream", "1")
       sst.addcomplink(l1id, "cpu_cache_link_" + str(x), "upstream0", "1ns")
       sst.addcomplink(l1id, "cache_bus_link_" + str(x), "snoop_link", "50ps")
       sst.addcompparam(l1id, "printStats", "1")
       sst.addcomplink(membus, "cache_bus_link_" + str(x), "port" + str(x), "50ps")
       print "added cache_bus_link_%d at port %d\n"%(x,x)

    # Put a link to memory from the memory bus
    sst.addcomplink(membus, "mem_bus_link", "port%d"%(corecount), "50ps")

    memory = sst.createcomponent("memory", "memHierarchy.MemController")
    sst.addcompparam(memory, "access_time", "1000ns")
    sst.addcompparam(memory, "mem_size", "512")
    sst.addcompparam(memory, "clock", "1GHz")
    sst.addcompparam(memory, "use_dramsim", "0")
    sst.addcompparam(memory, "device_ini", "DDR3_micron_32M_8B_x4_sg125.ini")
    sst.addcompparam(memory, "system_ini", "system.ini")
    sst.addcomplink(memory, "mem_bus_link", "snoop_link", "50ps")

    return 0
