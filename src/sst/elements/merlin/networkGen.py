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

import sst
from sst.merlin import *

if __name__ == "__main__":
    topos = dict( [(1,topoTorus()), (2,topoFatTree()), (3,topoDragonFly()), (4,topoSimple()), (5,topoMesh()), (6,topoDragonFly2()) ])
    endpoints = dict([(1,TestEndPoint()), (2, TrafficGenEndPoint()), (3, BisectionEndPoint())])
    statoutputs = dict([(1,"sst.statOutputConsole"), (2,"sst.statOutputCSV"), (3,"sst.statOutputTXT")]) 

    
    
    print "Merlin SDL Generator\n"


    print "Please select network topology:"
    for (x,y) in topos.iteritems():
        print "[ %d ]  %s" % (x, y.getName() )
    topo = int(raw_input())
    if topo not in topos:
        print "Bad answer.  try again."
        sys.exit(1)

    topo = topos[topo]



    print "Please select endpoint:"
    for (x,y) in endpoints.iteritems():
        print "[ %d ]  %s" % (x, y.getName() )
    ep = int(raw_input())
    if ep not in endpoints:
        print "Bad answer. try again."
        sys.exit(1)


    endPoint = endpoints[ep];

    sst.merlin._params["xbar_arb"] = "merlin.xbar_arb_lru"

    
    print "Set statistics load level (0 = off):"
    stats = int(raw_input())
    if ( stats != 0 ):
        print "Statistic dump period (0 = end of sim only):"
        rate = raw_input();
        if ( rate == "" ):
            rate = "0"
        sst.setStatisticLoadLevel(stats)
        
        print "Please select statistics output type:"
        for (x,y) in statoutputs.iteritems():
            print "[ %d ]  %s" % (x, y)
        output = int(raw_input())
        if output not in statoutputs:
            print "Bad answer.  try again."
            sys.exit(1)
        
        sst.setStatisticOutput(statoutputs[output]);
        if (output != 1):
            print "Filename for stats output:"
            filename = raw_input()
            sst.setStatisticOutputOptions({
                    "filepath" : filename,
                    "separator" : ", "
                    })

        endPoint.enableAllStatistics(rate)


    topo.prepParams()
    endPoint.prepParams()
    topo.setEndPoint(endPoint)
    topo.build()

    if ( stats != 0 ):
        sst.enableAllStatisticsForComponentType("merlin.hr_router", {"type":"sst.AccumulatorStatistic",
                                                                     "rate":rate});
        #stats.append("port%d_send_bit_count"%l)
        #stats.append("port%d_send_packet_count"%l)
        #stats.append("port%d_xbar_stalls"%l)

