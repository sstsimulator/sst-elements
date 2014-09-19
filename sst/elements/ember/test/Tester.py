#! /usr/bin/env python

import sys,os
from subprocess import call
import CrossProduct
from CrossProduct import *

config = "emberLoad.py"

tests = []
networks = [] 

net = { 'topo' : 'torus',
        'args' : [ 
                    [ '--shape', ['2','4x4x4','8x8x8','16x16x16'] ] 
                 ]
      }

networks.append(net);

net = { 'topo' : 'fattree',
        'args' : [  
                    ['--radix',   ['16']],
                    ['--loading', ['8']]
                 ]
      }

networks.append(net);

test = { 'motif' : 'AllPingPong',
         'args'  : [ 
                        [ 'iterations'  , ['1','10']],
                        [ 'messageSize' , ['0','1','10000','20000']] 
                   ] 
        }

tests.append( test )

test = { 'motif' : 'Allreduce',
         'args'  : [  
                        [ 'iterations'  , ['1','10']],
                        [ 'count' , ['1']] 
                   ] 
        }

tests.append( test )

test = { 'motif' : 'Barrier',
         'args'  : [  
                        [ 'iterations'  , ['1','10']]
                   ] 
        }

tests.append( test )

test = { 'motif' : 'PingPong',
         'args'  : [  
                        [ 'iterations'  , ['1','10']],
                        [ 'messageSize' , ['0','1','10000','20000']] 
                   ] 
        }

tests.append( test )

test = { 'motif' : 'Reduce',
         'args'  : [  
                        [ 'iterations'  , ['1','10']],
                        [ 'count' , ['1']] 
                   ] 
        }

tests.append( test )

test = { 'motif' : 'Ring',
         'args'  : [  
                        [ 'iterations'  , ['1','10']],
                        [ 'messagesize' , ['0','1','10000','20000']] 
                   ] 
        }

tests.append( test )

for network in networks :
    for test in tests :
        for x in CrossProduct( network['args'] ) :
            for y in CrossProduct( test['args'] ):
                print "sst --model-options=\"--topo={0} {1} --cmdLine=\\\"{2} {3}\\\"\" {4}".format(network['topo'], x, test['motif'], y, config)
                #call("sst --model-options=\"--topo={0} {1} --cmdLine=\\\"{2} {3}\\\"\" {4}".format(network['topo'], x, test['motif'], y, config), shell=True )
