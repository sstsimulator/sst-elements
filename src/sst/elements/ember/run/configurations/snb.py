import sys,pprint
import sst

pp = pprint.PrettyPrinter(indent=4)


def _createRouters( prefix, numRouters, ringstop_params) :
    router_map = {} 
    for next_ring_stop in range( numRouters ):

        name = prefix + "rtr." + str(next_ring_stop)

        ring_rtr = sst.Component(name, "merlin.hr_router")
        ring_rtr.addParams(ringstop_params)
        ring_rtr.addParams({
                "id" : next_ring_stop
                })
        router_map[name] = ring_rtr
        #print 'add', name

    return router_map


def _wireRouters( prefix, router_map, ring_latency ):

    numRouters = len(router_map)

    for curStop in range(numRouters):
        nextStop = (curStop + 1) % numRouters
        prevStop = (curStop - 1) % numRouters

        rtr_link_positive = sst.Link(prefix + "rtr_pos_" + str(curStop))
        rtr_link_positive.connect( 
                        (router_map[prefix + "rtr." + str(curStop)], "port0", ring_latency), 
                        (router_map[prefix + "rtr." + str(nextStop)], "port1", ring_latency) 
                    )

        rtr_link_negative = sst.Link(prefix + "rtr_neg_" + str(curStop))
        rtr_link_negative.connect( 
                        (router_map[prefix + "rtr." + str(curStop)], "port1", ring_latency), 
                        (router_map[prefix + "rtr." + str(prevStop)], "port0", ring_latency) 
                    )

def _configureL1L2(prefix, id, l1_params, l1_prefetch_params, l2_params, l2_prefetch_params,
            network_id, ring_latency, rtr ):

    name = prefix + "l1cache_" + id
    #print 'create', name
    l1 = sst.Component(name, "memHierarchy.Cache")
    l1.addParams(l1_params)
    l1.addParams(l1_prefetch_params)

    name = prefix + "l2cache_" + id
    #print 'create', name
    l2 = sst.Component(name, "memHierarchy.Cache")
    l2.addParams({ 
        "network_address" : network_id })
    l2.addParams(l2_params)
    l2.addParams(l2_prefetch_params)

    name = prefix + "l2cache_" + id+ "_link"
    #print 'create', name
    l2_core_link = sst.Link(name)
    l2_core_link.connect((l1, "low_network_0", ring_latency), (l2, "high_network_0", ring_latency))

    name = prefix + "l2_ring_link_" + id

    #print 'create', name
    l2_ring_link = sst.Link(name)
    l2_ring_link.connect((l2, "cache", ring_latency), (rtr, "port2", ring_latency))
    return l1


#ariel_cache_link = sst.Link("ariel_cache_link_" + str(next_core_id))
#ariel_cache_link.connect( (ariel, "cache_link_" + str(next_core_id), ring_latency), (l1, "high_network_0", ring_latency) )

def _configureL3( prefix, id, l3_params, network_id, ring_latency, rtr ):


    name = prefix + "l3cache_" +  id
    #print 'create', name
    l3cache = sst.Component(name, "memHierarchy.Cache")
    l3cache.addParams(l3_params)

    l3cache.addParams({
        "network_address" : network_id,
        "slice_id" : id
    })

    name = prefix + "l3_ring_link_" + id
    #print 'create', name
    l3_ring_link = sst.Link(name)
    l3_ring_link.connect( (l3cache, "directory", ring_latency), (rtr, "port2", ring_latency) )



def _configureMemCtrl( prefix, id, dc_params, mem_params, start_addr, end_addr, network_id, ring_latency, rtr ): 

    name = prefix + "memory_" + id 
    #print 'create', name
    mem = sst.Component(name, "memHierarchy.MemController")
    mem.addParams(mem_params)

    name = prefix + "dc_" + id 
    #print 'create' , name
    dc = sst.Component(name, "memHierarchy.DirectoryController")
    dc.addParams({
        "network_address" : network_id,
        "addr_range_start" : start_addr,
        "addr_range_end" : end_addr,
    })
    dc.addParams(dc_params)

    name = prefix + "mem_link_" + id
    #print 'create', name
    memLink = sst.Link(name)
    memLink.connect((mem, "direct_link", ring_latency), (dc, "memory", ring_latency))

    name = prefix + "dc_ring_link_" + id
    #print 'create', name
    netLink = sst.Link(name)
    netLink.connect((dc, "network", ring_latency), (rtr, "port2", ring_latency))


def _configCache( prefix, 
        router_map,
        groups, cores_per_group,
        l1_params, l1_prefetch_params,
        l2_params, l2_prefetch_params,
        l3_params, l3cache_blocks_per_group,
        memory_controllers_per_group, memory_capacity, mem_interleave_size,
        mem_params,
        dc_params,
        ring_latency ):

    next_network_id = 0
    next_core_id = 0
    next_memory_ctrl_id = 0
    cpuL1s = []

    for next_group in range(groups):
        print "Configuring core and memory controller group " + str(next_group) + "..."

        print 'Create group {0}'.format( next_group) 


        for next_active_core in range(cores_per_group):

            print 'Creating L1/L2: ' + str(next_core_id) + ' in group: ' + str(next_group)
             
            cpuL1s += [ _configureL1L2( prefix, str(next_core_id), l1_params, l1_prefetch_params,
                                l2_params, l2_prefetch_params, next_network_id, ring_latency,
                                router_map[prefix + "rtr." + str(next_network_id)]) ]

            next_network_id = next_network_id + 1
            next_core_id = next_core_id + 1

        for next_l3_cache_block in range(l3cache_blocks_per_group):

            print "Creating L3 cache block: " + str(next_l3_cache_block) + " in group: " + str(next_group)

            _configureL3( prefix, str( (next_group * l3cache_blocks_per_group) + next_l3_cache_block ),
                    l3_params,
                    next_network_id, ring_latency, router_map[prefix + "rtr." + str(next_network_id)]) 

            next_network_id = next_network_id + 1


        for next_mem_ctrl in range(memory_controllers_per_group):

            print "Creating Memory Controler: " + str(next_mem_ctrl) + ' in group: ' + str(next_group)
            local_size = memory_capacity / (groups * memory_controllers_per_group)

            start_addr = next_memory_ctrl_id * mem_interleave_size
            end_addr = (memory_capacity * 1024 * 1024) - \
                        (groups * memory_controllers_per_group * mem_interleave_size) + \
                        (next_memory_ctrl_id * mem_interleave_size)

            _configureMemCtrl( prefix, str(next_memory_ctrl_id),
                    dc_params,
                    mem_params,
                    start_addr, end_addr,
                    next_network_id, ring_latency,
                    router_map[prefix + "rtr." + str(next_network_id)] )

            next_network_id = next_network_id + 1
            next_memory_ctrl_id = next_memory_ctrl_id + 1

    print 'Creating NIC L1/L2'

    nicL1_read = _configureL1L2( prefix, 'nic_read', l1_params, l1_prefetch_params,
                                l2_params, l2_prefetch_params, next_network_id, ring_latency,
                                router_map[prefix + "rtr." + str(next_network_id)])
    
    next_network_id = next_network_id + 1
    nicL1_write = _configureL1L2( prefix, 'nic_write', l1_params, l1_prefetch_params,
                                l2_params, l2_prefetch_params, next_network_id, ring_latency,
                                router_map[prefix + "rtr." + str(next_network_id)])

    return cpuL1s, nicL1_read, nicL1_write


def configure( prefix, params ):


    routers = _createRouters( prefix, params['num_routers'], params['rtr_params'] )
    _wireRouters( prefix, routers, params['ring_latency'] )
    return _configCache( prefix, routers,
                    params['groups'],
                    params['cores_per_group'],
                    params['l1_params'],
                    params['l1_prefetch_params'],
                    params['l2_params'],
                    params['l2_prefetch_params'],
                    params['l3_params'],
                    params['l3cache_blocks_per_group'],
                    params['memory_controllers_per_group'],
                    params['memory_capacity'],
                    params['mem_interleave_size'],
                    params['mem_params'],
                    params['dc_params'],
                    params['ring_latency'] )
