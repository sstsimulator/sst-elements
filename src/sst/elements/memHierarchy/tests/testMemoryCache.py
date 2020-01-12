import os
import sst

# Debug parameters
quiet = True
debugAll = 0
debugL1 = max(debugAll, 0)
debugL2 = max(debugAll, 0)
debugDDRDC = max(debugAll, 0)
debugMemCtrl = max(debugAll, 0)
debugDDRMemCtrl = max(debugMemCtrl, 0)
debugHBMMemCtrl = max(debugMemCtrl, 0)
debugNIC = max(debugAll, 0)
debugLev = 10

# Parameters
hbmCapacity = 8    # In GB
ddrCapacity = 48    # In GB -> needs to be 2^x * 6
hbmPageSize = 4     # In KB
hbmNumPages = hbmCapacity * 1024 * 1024 / hbmPageSize
ddrPageSize = 4     # In KB
ddrNumPages = ddrCapacity * 1024 * 1024 / ddrPageSize

corecount = 72
core_clock = "2GHz"

mesh_clock = 1600
mesh_stops_x        = 6
mesh_stops_y        = 6

ctrl_mesh_flit      = 8
data_mesh_flit      = 36
mesh_link_latency   = "100ps"    # Note, used to be 50ps, didn't seem to make a difference when bumping it up to 100
ctrl_mesh_link_bw   = str( mesh_clock * 1000 * 1000 * ctrl_mesh_flit ) + "B/s"
data_mesh_link_bw   = str( mesh_clock * 1000 * 1000 * data_mesh_flit ) + "B/s"

coherence_protocol = "MESI"

ctrl_network_buffers = "32B"
data_network_buffers = "288B"

ctrl_network_params = {
    "link_bw" : ctrl_mesh_link_bw,
    "flit_size" : str(ctrl_mesh_flit) + "B",
    "input_buf_size" : ctrl_network_buffers,
    "route_y_first" : 1,
}

data_network_params = {
    "link_bw" : data_mesh_link_bw,
    "flit_size" : str(data_mesh_flit) + "B",
    "input_buf_size" : data_network_buffers,
    "route_y_first" : 1
}

nic_params = {
    "accept_region" : 0,
    "debug" : debugNIC,
    "debug_level" : debugLev,
}

ctrl_nic_params = {
    "link_bw" : ctrl_mesh_link_bw,
    "in_buf_size" : ctrl_network_buffers,
    "out_buf_size" : ctrl_network_buffers,
}

data_nic_params = {
    "link_bw" : data_mesh_link_bw,
    "in_buf_size" : data_network_buffers,
    "out_buf_size" : data_network_buffers,
}

l1_cache_params = {
    "cache_frequency"    : core_clock,
    "coherence_protocol" : coherence_protocol,
    "replacement_policy" : "lru",
    "cache_size"         : "32KiB",
    "associativity"      : 8,
    "cache_line_size"    : 64,
    "access_latency_cycles" : 3,
    "tag_access_latency_cycles" : 1,
    "mshr_num_entries"   : 24, # L1
    "maxRequestDelay"    : 1000000,
    "events_up_per_cycle" : 2,
    "mshr_latency_cycles" : 2, # L1
    "max_requests_per_cycle" : 4,
    "L1"                 : 1,
    "debug"              : debugL1,
    "debug_level"        : debugLev,
    "verbose"            : 2,
}

l2_prefetch_params = {
    "prefetcher" : "cassini.StridePrefetcher",
    "prefetcher.reach" : 16,
    "prefetcher.detect_range" : 1
}

l2_cache_params = {
    "cache_frequency"    : core_clock,
    "coherence_protocol" : coherence_protocol,
    "replacement_policy" : "lru",
    "cache_size"         : "1MiB",
    "associativity"      : 16,
    "cache_line_size"    : 64,
    "access_latency_cycles" : 8,
    "tag_access_latency_cycles" : 2,
    "mshr_num_entries"   : 64, # L2
    "mshr_latency_cycles" : 4, # L2
    "response_link_width" : "144B",
    "debug"              : debugL2,
    "debug_level"        : debugLev,
    "verbose"            : 2,
}

dc_params = {
    "coherence_protocol": coherence_protocol,
    "clock"             : str(mesh_clock) + "MHz",
    "verbose"            : 2,
    "entry_cache_size"  : 256*1024*1024, #Entry cache size of mem/blocksize
    "mshr_num_entries"  : 256, # DDR DC
    "access_latency_cycles" : 2,
    "debug"             : debugDDRDC,
    "debug_level"       : debugLev
}

##### HBM controller #########
hbm_memcache_td_params = {
    "clock"     : "1.2GHz",
    "verbose"   : 2,
    "max_requests_per_cycle" : 8,
    "backing" : "none",
    "cache_line_size" : 64,
    "debug" : debugHBMMemCtrl,
    "debug_level" : 3,
}

hbm_td_backend_params = {
    "id" : 0,
    "addrMapper" : "memHierarchy.simpleAddrMapper",
    "channels" : 16, # Chan *pChan
    "channel.transaction_Q_size" : 64,
    "channel.numRanks" : 1,
    "channel.rank.numBanks" : 16,
    "channel.rank.bank.CL" : 20,
    "channel.rank.bank.CL_WR" : 7,
    "channel.rank.bank.RCD" : 14,
    "channel.rank.bank.TRP" : 14,
    "channel.rank.bank.dataCycles" : 4,
    "channel.rank.bank.pagePolicy" : "memHierarchy.simplePagePolicy",
    "channel.rank.bank.transactionQ" : "memHierarchy.reorderTransactionQ",
    "channel.rank.bank.pagePolicy.close" : 0,
    "printconfig" : 0,
    "channel.printconfig" : 0,
    "channel.rank.printconfig" : 0,
    "channel.rank.bank.printconfig" : 0
}

#### DDR controller #########
ddr_mem_params = {
    "backing" : "none",
    "clock" : "1.2GHz",
    "verbose"            : 2,
    "debug" : debugDDRMemCtrl,
    "debug_level" : 3,
}

ddr_td_backend_params = {
    "id" : 0,
    "addrMapper" : "memHierarchy.simpleAddrMapper",
    "channel.transaction_Q_size" : 32,
    "channel.numRanks" : 2,
    "channel.rank.numBanks" : 16,
    "channel.rank.bank.CL" : 15,
    "channel.rank.bank.CL_WR" : 12,
    "channel.rank.bank.RCD" : 15,
    "channel.rank.bank.TRP" : 15,
    "channel.rank.bank.dataCycles" : 4,
    "channel.rank.bank.pagePolicy" : "memHierarchy.simplePagePolicy",
    "channel.rank.bank.transactionQ" : "memHierarchy.reorderTransactionQ",
    "channel.rank.bank.pagePolicy.close" : 0,
    "printconfig" : 0,
    "channel.printconfig" : 0,
    "channel.rank.printconfig" : 0,
    "channel.rank.bank.printconfig" : 0
}

##### Processor #####
thread_iters = 1000
core_params = {
    "verbose" : 0,
    "clock" : core_clock,
    "max_reqs_cycle" : 2,
    "maxmemreqpending" : 20,
    "pagecount" : ddrNumPages,
    "pagesize" : ddrPageSize * 1024
}

gen_params = {
    "verbose" : 0,
    "n" : thread_iters,
    "operandWidth" : 8,
}


class DDRBuilder:
    def __init__(self, memCapacity, startAddr, ddrCount):
        self.next_ddr_id = 0
        self.ddr_count = ddrCount
        self.mem_capacity = memCapacity
        self.start_addr = startAddr

    def build(self, nodeID):
        if not quiet:
            print "Creating DDR controller " + str(self.next_ddr_id) + " out of " + str(self.ddr_count) + " on node " + str(nodeID) + "..."
            print " - Capacity: " + str(self.mem_capacity / self.ddr_count) + " per DDR."

        mem = sst.Component("ddr_" + str(self.next_ddr_id), "memHierarchy.MemController")
        mem.addParams(ddr_mem_params)

        membk = mem.setSubComponent("backend", "memHierarchy.timingDRAM")
        membk.addParams(ddr_td_backend_params)
        membk.addParams({ "mem_size" : str(self.mem_capacity / self.ddr_count) + "B" })

        mem.addParams({
            "addr_range_start" : self.start_addr + (64 * self.next_ddr_id),
            "addr_range_end" : self.start_addr + (self.mem_capacity - (64 * self.next_ddr_id)),
            "interleave_step" : str(self.ddr_count * 64) + "B",
            "interleave_size" : "64B",
        })

        # Define DDR NIC
        mlink = mem.setSubComponent("cpulink", "memHierarchy.MemNICFour")
        data = mlink.setSubComponent("data", "kingsley.linkcontrol")
        req = mlink.setSubComponent("req", "kingsley.linkcontrol")
        fwd = mlink.setSubComponent("fwd", "kingsley.linkcontrol")
        ack = mlink.setSubComponent("ack", "kingsley.linkcontrol")
        mlink.addParams(nic_params)
        mlink.addParams({"group" : 4})
        data.addParams(data_nic_params)
        req.addParams(ctrl_nic_params)
        fwd.addParams(ctrl_nic_params)
        ack.addParams(ctrl_nic_params)

        self.next_ddr_id = self.next_ddr_id + 1
        return (req, "rtr_port", mesh_link_latency), (ack, "rtr_port", mesh_link_latency), (fwd, "rtr_port", mesh_link_latency), (data, "rtr_port", mesh_link_latency)


class DCBuilder:
    def __init__(self, memCapacity, dcCount):
        self.next_dc_id = 0
        self.dc_count = dcCount
        self.memCapacity = memCapacity

    def build(self, nodeID):
        myStart = self.next_dc_id * 64
        myEnd = self.memCapacity - (self.dc_count *64) + myStart + 63
            
        if not quiet:
            print "\tCreating dc with start: " + str(myStart) + " end: " + str(myEnd)
        
        # "Home" memory for each dc just desginates where it is caching its entries:
        # ddr0: 0-2, 6-8
        # ddr1: 3-5, 9-11
        # ddr2: 12-14, 18-20
        # ddr3: 15-17, 21-23
        # ddr4: 24-26, 30-32
        # ddr5: 27-29, 33-35
        ddrnum = 2 * (self.next_dc_id // 12) + (self.next_dc_id // 3) % 2

        dc = sst.Component("dc_" + str(self.next_dc_id), "memHierarchy.DirectoryController")
        dc.addParams(dc_params)

        dc.addParams({
            "addr_range_start" : myStart,
            "addr_range_end" : myEnd,
            "interleave_step" : str(self.dc_count * 64) + "B",
            "interleave_size" : "64B",
        })

        # Define DC NIC
        dclink = dc.setSubComponent("cpulink", "memHierarchy.MemNICFour")
        data = dclink.setSubComponent("data", "kingsley.linkcontrol")
        req = dclink.setSubComponent("req", "kingsley.linkcontrol")
        fwd = dclink.setSubComponent("fwd", "kingsley.linkcontrol")
        ack = dclink.setSubComponent("ack", "kingsley.linkcontrol")
        dclink.addParams(nic_params)
        dclink.addParams({"group" : 2})
        data.addParams(data_nic_params)
        req.addParams(ctrl_nic_params)
        fwd.addParams(ctrl_nic_params)
        ack.addParams(ctrl_nic_params)

        self.next_dc_id = self.next_dc_id + 1
        return (req, "rtr_port", mesh_link_latency), (ack, "rtr_port", mesh_link_latency), (fwd, "rtr_port", mesh_link_latency), (data, "rtr_port", mesh_link_latency)


class HBMBuilder:
    def __init__(self, memCapacity, hbmCount):
        self.next_hbm_id = 0
        self.hbm_count = hbmCount
        self.memCapacity = memCapacity

    def build(self, nodeID):
        if not quiet:
            print "Creating HBM controller " + str(self.next_hbm_id) + " out of " + str(self.hbm_count) + " on node " + str(nodeID) + "..."
            print " - Capacity: " + str(self.memCapacity / self.hbm_count) + " per HBM."

        mem = sst.Component("hbm_" + str(self.next_hbm_id), "memHierarchy.MemCacheController")
        mem.addParams(hbm_memcache_td_params)
        
        membk = mem.setSubComponent("backend", "memHierarchy.timingDRAM")
        membk.addParams(hbm_td_backend_params)
        membk.addParams({ "mem_size" : str(self.memCapacity / self.hbm_count) + "B" })

        memLink = sst.Link("hbm_link_" + str(self.next_hbm_id))

        mem.addParams({
            "num_caches" : self.hbm_count,
            "cache_num" : self.next_hbm_id,
        })
        
        # Define HBM NIC
        mlink = mem.setSubComponent("cpulink", "memHierarchy.MemNICFour")
        data = mlink.setSubComponent("data", "kingsley.linkcontrol")
        req = mlink.setSubComponent("req", "kingsley.linkcontrol")
        fwd = mlink.setSubComponent("fwd", "kingsley.linkcontrol")
        ack = mlink.setSubComponent("ack", "kingsley.linkcontrol")
        mlink.addParams(nic_params)
        mlink.addParams({"group" : 3})
        data.addParams(data_nic_params)
        req.addParams(ctrl_nic_params)
        fwd.addParams(ctrl_nic_params)
        ack.addParams(ctrl_nic_params)

        self.next_hbm_id = self.next_hbm_id + 1
        return (req, "rtr_port", mesh_link_latency), (ack, "rtr_port", mesh_link_latency), (fwd, "rtr_port", mesh_link_latency), (data, "rtr_port", mesh_link_latency)


class TileBuilder:
    def __init__(self):
        self.next_tile_id = 0
        self.next_core_id = 0
        self.base_a = 0
        self.base_b = thread_iters * 8 * 36
        self.base_c = self.base_b + thread_iters * 8 * 36

    def build(self, nodeID):
        tileL2cache = sst.Component("l2cache_" + str(self.next_tile_id), "memHierarchy.Cache")
        tileL2cache.addParams(l2_cache_params)
        tileL2cache.addParams(l2_prefetch_params)
        
        # Define L2 NIC
        l2clink = tileL2cache.setSubComponent("cpulink", "memHierarchy.MemLink")
        l2mlink = tileL2cache.setSubComponent("memlink", "memHierarchy.MemNICFour")
        l2data = l2mlink.setSubComponent("data", "kingsley.linkcontrol")
        l2req = l2mlink.setSubComponent("req", "kingsley.linkcontrol")
        l2fwd = l2mlink.setSubComponent("fwd", "kingsley.linkcontrol")
        l2ack = l2mlink.setSubComponent("ack", "kingsley.linkcontrol")
        l2mlink.addParams(nic_params)
        l2mlink.addParams({"group" : 1})
        l2data.addParams(data_nic_params)
        l2req.addParams(ctrl_nic_params)
        l2fwd.addParams(ctrl_nic_params)
        l2ack.addParams(ctrl_nic_params)

        # Bus between L2 and L1s on the tile
        l2bus = sst.Component("l2cachebus_" + str(self.next_tile_id), "memHierarchy.Bus")
        l2bus.addParams({
            "bus_frequency" : core_clock,
            })

        l2busLink = sst.Link("l2bus_link_" + str(self.next_tile_id))
        l2busLink.connect( (l2bus, "low_network_0", mesh_link_latency), (l2clink, "port", mesh_link_latency))
        l2busLink.setNoCut()

        # L1s
        self.next_tile_id = self.next_tile_id + 1

        tileLeftL1 = sst.Component("l1cache_" + str(self.next_core_id), "memHierarchy.Cache")
        tileLeftL1.addParams(l1_cache_params)

        if not quiet:
            print "Creating core " + str(self.next_core_id) + " on tile: " + str(self.next_tile_id) + "..."

        leftL1L2link = sst.Link("l1cache_link_" + str(self.next_core_id))
        leftL1L2link.connect( (l2bus, "high_network_0", mesh_link_latency),
            (tileLeftL1, "low_network_0", mesh_link_latency))
        leftL1L2link.setNoCut()

        leftCore = sst.Component("core_" + str(self.next_core_id), "miranda.BaseCPU")
        leftCore.addParams(core_params)
        leftGen = leftCore.setSubComponent("generator", "miranda.STREAMBenchGenerator")
        leftGen.addParams(gen_params)
        leftGen.addParams({
            "start_a" : self.base_a + self.next_core_id * thread_iters * 8,
            "start_b" : self.base_b + self.next_core_id * thread_iters * 8,
            "start_c" : self.base_c + self.next_core_id * thread_iters * 8,
        })

        leftCoreL1link = sst.Link("core_link_" + str(self.next_core_id))
        leftCoreL1link.connect( (leftCore, "cache_link", mesh_link_latency), (tileLeftL1, "high_network_0", mesh_link_latency) )
        leftCoreL1link.setNoCut()

        self.next_core_id = self.next_core_id + 1

        tileRightL1 = sst.Component("l1cache_" + str(self.next_core_id), "memHierarchy.Cache")
        tileRightL1.addParams(l1_cache_params)
        
        rightCore = sst.Component("core_" + str(self.next_core_id), "miranda.BaseCPU")
        rightCore.addParams(core_params)
        rightGen = rightCore.setSubComponent("generator", "miranda.STREAMBenchGenerator")
        rightGen.addParams(gen_params)
        rightGen.addParams({
            "start_a" : self.base_a + self.next_core_id * thread_iters * 8,
            "start_b" : self.base_b + self.next_core_id * thread_iters * 8,
            "start_c" : self.base_c + self.next_core_id * thread_iters * 8,
        })
        
        rightCoreL1link = sst.Link("core_link_" + str(self.next_core_id))
        rightCoreL1link.connect( (rightCore, "cache_link", mesh_link_latency), (tileRightL1, "high_network_0", mesh_link_latency) )
        rightCoreL1link.setNoCut()

        if not quiet:
            print "Creating core " + str(self.next_core_id) + " on tile: " + str(self.next_tile_id) + "..."
        
        rightL1L2link = sst.Link("l1cache_link_" + str(self.next_core_id))
        rightL1L2link.connect( (l2bus, "high_network_1", mesh_link_latency),
                        (tileRightL1, "low_network_0", mesh_link_latency))
        rightL1L2link.setNoCut()

        self.next_core_id = self.next_core_id + 1

        return (l2req, "rtr_port", mesh_link_latency), (l2ack, "rtr_port", mesh_link_latency), (l2fwd, "rtr_port", mesh_link_latency), (l2data, "rtr_port", mesh_link_latency)

######## Distributed Directory Mapping #########

tileBuilder = TileBuilder()
hbmBuilder  = HBMBuilder(hbmCapacity * 1024 * 1024 * 1024, 8)
ddrBuilder  = DDRBuilder(ddrCapacity * 1024 * 1024 * 1024, 0, 6)
dcBuilder   = DCBuilder(ddrCapacity * 1024 * 1024 * 1024, 36)

def setNode(nodeId, rtrreq, rtrack, rtrfwd, rtrdata):
    if nodeId == 0 or nodeId == 1 or nodeId == 4 or nodeId == 5:
        hbmreq, hbmack, hbmfwd, hbmdata = hbmBuilder.build(nodeId)
        rtrreqnorth = sst.Link("krtr_req_north_" + str(nodeId))
        rtrreqnorth.connect( (rtrreq, "north", mesh_link_latency), hbmreq )
        rtracknorth = sst.Link("krtr_ack_north_" + str(nodeId))
        rtracknorth.connect( (rtrack, "north", mesh_link_latency), hbmack )
        rtrfwdnorth = sst.Link("krtr_fwd_north_" + str(nodeId))
        rtrfwdnorth.connect( (rtrfwd, "north", mesh_link_latency), hbmfwd )
        rtrdatanorth = sst.Link("kRtr_data_north_" + str(nodeId))
        rtrdatanorth.connect( (rtrdata, "north", mesh_link_latency), hbmdata )
    elif nodeId == 30 or nodeId == 31 or nodeId == 34 or nodeId == 35:
        hbmreq, hbmack, hbmfwd, hbmdata = hbmBuilder.build(nodeId)
        rtrreqsouth = sst.Link("krtr_req_south_" + str(nodeId) )
        rtrreqsouth.connect( (rtrreq, "south", mesh_link_latency), hbmreq )
        rtracksouth = sst.Link("krtr_ack_south_" + str(nodeId) )
        rtracksouth.connect( (rtrack, "south", mesh_link_latency), hbmack )
        rtrfwdsouth = sst.Link("krtr_fwd_south_" + str(nodeId) )
        rtrfwdsouth.connect( (rtrfwd, "south", mesh_link_latency), hbmfwd )
        rtrdatasouth = sst.Link("kRtr_data_south_" + str(nodeId) )
        rtrdatasouth.connect( (rtrdata, "south", mesh_link_latency), hbmdata )
    elif nodeId == 6 or nodeId == 12 or nodeId == 18:
        ddrreq, ddrack, ddrfwd, ddrdata = ddrBuilder.build(nodeId)
        rtrreqwest = sst.Link("krtr_req_west_" + str(nodeId) )
        rtrreqwest.connect( (rtrreq, "west", mesh_link_latency), ddrreq )
        rtrackwest = sst.Link("krtr_ack_west_" + str(nodeId) )
        rtrackwest.connect( (rtrack, "west", mesh_link_latency), ddrack )
        rtrfwdwest = sst.Link("krtr_fwd_west_" + str(nodeId) )
        rtrfwdwest.connect( (rtrfwd, "west", mesh_link_latency), ddrfwd )
        rtrdatawest = sst.Link("kRtr_data_west_" + str(nodeId) )
        rtrdatawest.connect( (rtrdata, "west", mesh_link_latency), ddrdata )
    elif nodeId == 11 or nodeId == 17 or nodeId == 23:
        ddrreq, ddrack, ddrfwd, ddrdata = ddrBuilder.build(nodeId)
        rtrreqeast = sst.Link("krtr_req_east_" + str(nodeId) )
        rtrreqeast.connect( (rtrreq, "east", mesh_link_latency), ddrreq )
        rtrackeast = sst.Link("krtr_ack_east_" + str(nodeId) )
        rtrackeast.connect( (rtrack, "east", mesh_link_latency), ddrack )
        rtrfwdeast = sst.Link("krtr_fwd_east_" + str(nodeId) )
        rtrfwdeast.connect( (rtrfwd, "east", mesh_link_latency), ddrfwd )
        rtrdataeast = sst.Link("kRtr_data_east_" + str(nodeId) )
        rtrdataeast.connect( (rtrdata, "east", mesh_link_latency), ddrdata )

    # Place tiles on all routers
    tilereq, tileack, tilefwd, tiledata = tileBuilder.build(nodeId)
    reqport0 = sst.Link("krtr_req_port0_" + str(nodeId))
    reqport0.connect( (rtrreq, "local0", mesh_link_latency), tilereq )
    ackport0 = sst.Link("krtr_ack_port0_" + str(nodeId))
    ackport0.connect( (rtrack, "local0", mesh_link_latency), tileack )
    fwdport0 = sst.Link("krtr_fwd_port0_" + str(nodeId))
    fwdport0.connect( (rtrfwd, "local0", mesh_link_latency), tilefwd )
    dataport0 = sst.Link("kRtr_data_port0_" + str(nodeId))
    dataport0.connect( (rtrdata, "local0", mesh_link_latency), tiledata )

    dcreq, dcack, dcfwd, dcdata = dcBuilder.build(nodeId)
    reqport1 = sst.Link("krtr_req_port1_" + str(nodeId))
    reqport1.connect( (rtrreq, "local1", mesh_link_latency), dcreq)
    ackport1 = sst.Link("krtr_ack_port1_" + str(nodeId))
    ackport1.connect( (rtrack, "local1", mesh_link_latency), dcack)
    fwdport1 = sst.Link("krtr_fwd_port1_" + str(nodeId))
    fwdport1.connect( (rtrfwd, "local1", mesh_link_latency), dcfwd)
    dataport1 = sst.Link("kRtr_data_port1_" + str(nodeId))
    dataport1.connect( (rtrdata, "local1", mesh_link_latency), dcdata)


print "Building model..."

# Build the mesh using Kingsley -> duplicate mesh for data & control
kRtrReq=[]
kRtrAck=[]
kRtrFwd=[]
kRtrData=[]
for x in range (0, mesh_stops_x):
    for y in range (0, mesh_stops_y):
        nodeNum = len(kRtrReq)
        kRtrReq.append(sst.Component("krtr_req_" + str(nodeNum), "kingsley.noc_mesh"))
        kRtrReq[-1].addParams(ctrl_network_params)
        kRtrAck.append(sst.Component("krtr_ack_" + str(nodeNum), "kingsley.noc_mesh"))
        kRtrAck[-1].addParams(ctrl_network_params)
        kRtrFwd.append(sst.Component("krtr_fwd_" + str(nodeNum), "kingsley.noc_mesh"))
        kRtrFwd[-1].addParams(ctrl_network_params)
        kRtrData.append(sst.Component("krtr_data_" + str(nodeNum), "kingsley.noc_mesh"))
        kRtrData[-1].addParams(data_network_params)
        
        kRtrReq[-1].addParams({"local_ports" : 2})
        kRtrAck[-1].addParams({"local_ports" : 2})
        kRtrFwd[-1].addParams({"local_ports" : 2})
        kRtrData[-1].addParams({"local_ports" : 2})

i = 0
for y in range(0, mesh_stops_y):
    for x in range (0, mesh_stops_x):

        # North-south connections
        if y != (mesh_stops_y -1):
            kRtrReqNS = sst.Link("krtr_req_ns_" + str(i))
            kRtrReqNS.connect( (kRtrReq[i], "south", mesh_link_latency), (kRtrReq[i + mesh_stops_x], "north", mesh_link_latency) )
            kRtrAckNS = sst.Link("krtr_ack_ns_" + str(i))
            kRtrAckNS.connect( (kRtrAck[i], "south", mesh_link_latency), (kRtrAck[i + mesh_stops_x], "north", mesh_link_latency) )
            kRtrFwdNS = sst.Link("krtr_fwd_ns_" + str(i))
            kRtrFwdNS.connect( (kRtrFwd[i], "south", mesh_link_latency), (kRtrFwd[i + mesh_stops_x], "north", mesh_link_latency) )
            kRtrDataNS = sst.Link("krtr_data_ns_" + str(i))
            kRtrDataNS.connect( (kRtrData[i], "south", mesh_link_latency), (kRtrData[i + mesh_stops_x], "north", mesh_link_latency) )

        if x != (mesh_stops_x - 1):
            kRtrReqEW = sst.Link("krtr_req_ew_" + str(i))
            kRtrReqEW.connect( (kRtrReq[i], "east", mesh_link_latency), (kRtrReq[i+1], "west", mesh_link_latency) )
            kRtrAckEW = sst.Link("krtr_ack_ew_" + str(i))
            kRtrAckEW.connect( (kRtrAck[i], "east", mesh_link_latency), (kRtrAck[i+1], "west", mesh_link_latency) )
            kRtrFwdEW = sst.Link("krtr_fwd_ew_" + str(i))
            kRtrFwdEW.connect( (kRtrFwd[i], "east", mesh_link_latency), (kRtrFwd[i+1], "west", mesh_link_latency) )
            kRtrDataEW = sst.Link("krtr_data_ew_" + str(i))
            kRtrDataEW.connect( (kRtrData[i], "east", mesh_link_latency), (kRtrData[i+1], "west", mesh_link_latency) )

        setNode(i, kRtrReq[i], kRtrAck[i], kRtrFwd[i], kRtrData[i])
        i = i + 1

# Enable SST Statistics Outputs for this simulation
sst.setStatisticLoadLevel(16)
sst.enableAllStatisticsForAllComponents({"type":"sst.AccumulatorStatistic"})

#sst.setStatisticOutput("sst.statOutputCSV")
#sst.setStatisticOutputOptions( {
#        "filepath"  : "stat.csv",
#        "separator" : ", "
#} )

print "Model complete"
