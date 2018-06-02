#!/usr/bin/env python
#
# Copyright 2009-2018 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2018, NTESS
# All rights reserved.
#
# Portions are copyright of other developers:
# See the file CONTRIBUTORS.TXT in the top level directory
# the distribution for more information.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

import sys,sst,pprint

pp = pprint.PrettyPrinter(indent=4)

from detailedModel import *

class BasicDetailedModel(DetailedModel):
    def __init__(self, params ):
        self.name = 'BasicDetailedModel' 
        self.params = params

    def getName(self):
        return self.name

    def _createThreads(self, prefix, bus, numThreads, cpu_params, l1_params ):
        #print "createThreads() ", prefix 
        prefix += "thread"
        links = []
        for i in range( numThreads ) :
            name = prefix + str(i) + "_" 
            cpu = sst.Component( name + "cpu", "miranda.BaseCPU")
            cpu.addParams( cpu_params )
            print "l1"
            l1 = sst.Component( name + "l1cache", "memHierarchy.Cache")
            l1.addParams(l1_params)

            link = sst.Link( name + "cpu_l1_link")
            link.setNoCut();
            link.connect( ( cpu, "cache_link", "100ps" ) , (l1,"high_network_0","1000ps") )
            
            link = sst.Link( name + "l1_bus_link")
            link.setNoCut();
            link.connect( ( l1, "low_network_0", "50ps" ) , (bus,"high_network_" + str(i),"1000ps") )

            link = sst.Link( name + "src_link" )
            link.setNoCut();
            cpu.addLink( link, "src", "1ps" );
            links.append( link )

        return links

    def _createNic( self, prefix, bus, num, cpu_params, l1_params ):
        name = prefix + "nic_"
        #print "createNic() ", name

        cpu = sst.Component( name + "cpu", "miranda.BaseCPU")
        cpu.addParams( cpu_params )
        print "l1"
        l1 = sst.Component( name + "l1cache", "memHierarchy.Cache")
        l1.addParams( l1_params )

        link = sst.Link( name + "cpu_l1_link")
        link.setNoCut();
        link.connect( ( cpu, "cache_link", "100ps" ) , (l1,"high_network_0","1000ps") )

        link = sst.Link( name + "l1_bus_link")
        link.setNoCut();
        link.connect( ( l1, "low_network_0", "50ps" ) , (bus,"high_network_" + str(num),"1000ps") )

        link = sst.Link( name + "src_link" )
        link.setNoCut();
        cpu.addLink( link, "src", "1ps" );

        return link

    def build(self,nodeID,ransPerNode):
        #print 'BasicDetailedModel.build( nodeID={0}, ransPerNode={1} )'.format( nodeID, ransPerNode ) 

        pp.pprint( self.params )
        numThreads = int(self.params['numThreads']) 

        self.links = []
        self.nicLinks = []

        prefix = "basicModel_node" + str(nodeID) + "_"

        # Create Memory
        memory = sst.Component( prefix + "memory", "memHierarchy.MemController")
        memory.addParams( self.params['memory_params'])

        # Create L3
        l3 = sst.Component( prefix + "l3cache", "memHierarchy.Cache")
        l3.addParams( self.params['l3_params'])

        # Connect L3 to Memory    
        link = sst.Link( prefix + "l3_mem_link")
        link.setNoCut();
        link.connect( (l3, "low_network_0", "50ps"), (memory, "direct_link", "50ps") ) 


        # Create L2
        l2 = sst.Component( prefix + "l2cache", "memHierarchy.Cache")
        l2.addParams( self.params['l2_params'])

        # Connect L2 to L3 
        link = sst.Link( prefix + "l2_l3_link")
        link.setNoCut();
        link.connect( (l2, "low_network_0", "50ps"), (l3, "high_network_0", "50ps") ) 

        # Create Bus
        bus = sst.Component( prefix + "bus", "memHierarchy.Bus")
        bus.addParams( self.params['bus_params'])

        # Connect Bus to L2
        link = sst.Link( prefix + "bus_l2_link")
        link.setNoCut();
        link.connect( (bus, "low_network_0", "50ps"), (l2, "high_network_0", "50ps") ) 

        for i in range(ransPerNode):
            name = prefix + "core" + str(i) + "_" 
            self.links.append( \
                self._createThreads( name, bus, numThreads, self.params['cpu_params'], self.params['l1_params']  ) )
            
        self.nicLinks.append( self._createNic( prefix + 'read', bus, numThreads, self.params['nic_cpu_params'],\
                                    self.params['nic_l1_params'] ) )

        self.nicLinks.append( self._createNic( prefix + 'write', bus, numThreads+1, self.params['nic_cpu_params'],\
                                    self.params['nic_l1_params'] ) )

        return True 

    def getThreadLinks(self,core):
        #print 'BasicDetailedModel.getThreadLinks( {0} )'.format(core) 
        return self.links[core]

    def getNicLink(self ):
        #print 'BasicDetailedModel.getNicLink()' 
        return self.nicLinks[0], self.nicLinks[1] 

def getModel(params):
    return BasicDetailedModel(params)
