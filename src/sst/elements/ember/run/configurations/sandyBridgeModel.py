
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

import sst
import sys,pprint

pp = pprint.PrettyPrinter(indent=4)


from detailedModel import *

import snb

class SandyBridgeModel(DetailedModel):
    def __init__(self, params ):
        self.name = 'SandyBridgeModel' 
        self.params = params
        self.links = []

    def getName(self):
        return self.name

    def _createThreads( self, prefix, cpuL1s, cpu_params ):
        
        prefix += "thread"
        #print prefix
        links = []
        for i in range(len(cpuL1s)):
            name = prefix + str(i) 
            cpu = sst.Component( name , "miranda.BaseCPU")
            cpu.addParams( cpu_params )

            link = sst.Link( name + "_l1_link" )

            link.setNoCut();
            link.connect( ( cpu, "cache_link", "100ps" ) , (cpuL1s[i],"high_network_0","1000ps") )

            link = sst.Link( name + "_src_link" )
            link.setNoCut();
            cpu.addLink( link, "src", "1ps" );
            links.append( link )

        return links

    def _createNic( self, prefix, nicL1, cpu_params ):
        name = prefix + "nic_"
        #print "createNic() ", name

        cpu = sst.Component( name + "cpu", "miranda.BaseCPU")
        cpu.addParams( cpu_params )

        link = sst.Link( name + "cpu_l1_link")
        link.setNoCut();
        link.connect( ( cpu, "cache_link", "100ps" ) , (nicL1,"high_network_0","1000ps") )

        link = sst.Link( name + "src_link" )
        link.setNoCut();
        cpu.addLink( link, "src", "1ps" );

        return link

	def getName(self):
		return self._name;

    def build(self,nodeID,ranksPerNode):
        #print 'SandyBridgeModel.build( nodeID={0}, ranksPerNode={1} )'.format( nodeID, ranksPerNode ) 

        if ranksPerNode > 1:
            sys.exit('FATAL: this model can have onely 1 rank per node')

        self.links = []

        prefix = "sandyBridgeModel_node" + str(nodeID) + "_"

        pp.pprint( self.params )
        cpuL1s, nicL1_read, nicL1_write = snb.configure(prefix,self.params)

        self.links.append( self._createThreads( prefix, cpuL1s, self.params['cpu_params']  ) )

        self.nicLink = []
        self.nicLink.append( self._createNic( prefix + 'read', nicL1_read, self.params['nic_cpu_params'] ) )
        self.nicLink.append( self._createNic( prefix + 'write', nicL1_write, self.params['nic_cpu_params'] ) )

        return True 

    def getThreadLinks(self,nodeRank):
        #print 'SandyBridgeModel.getThreadLinks( {0} )'.format(nodeRank) 
        return self.links[nodeRank]

    def getNicLink(self ):
        #print 'SandyBridgeModel.getNicLink()' 
        return self.nicLink[0], self.nicLink[1] 

def getModel(params):
    return SandyBridgeModel(params)
