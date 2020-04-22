#! /usr/bin/env python

import sys,os
from subprocess import call
import CrossProduct
from CrossProduct import *
import hashlib
import binascii

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
                    ['--shape',   ['9,9:9,9:18']],
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
                hash_object  = hashlib.md5(b"sst --model-options=\"--topo={0} {1} --cmdLine=\\\"Init\\\" --cmdLine=\\\"{2} {3}\\\" --cmdLine=\\\"Fini\\\"\" {4}".format(network['topo'], x, test['motif'], y, config))
                hex_dig = hash_object.hexdigest()
                print ("test_SweepEmber_" + hex_dig + "() {")
                print ("echo \"    \" {0} {1} {2} {3}".format(network['topo'], x, test['motif'], y))
                print ("pushd $SST_ROOT/sst/elements/ember/test")
                print ("sst --model-options=\"--topo={0} {1} --cmdLine=\\\"Init\\\" --cmdLine=\\\"{2} {3}\\\" --cmdLine=\\\"Fini\\\"\" {4} > tmp_file".format(network['topo'], x, test['motif'], y, config))
                print ("grep Simulation.is.complete tmp_file > outFile ")
                print ("TL=`grep Simulation.is.complete tmp_file`")
                print ("echo $TL")
                print ("echo {0}   $TL >> $SST_TEST_OUTPUTS/SweepEmber_cumulative.out".format(hex_dig))
                #print ("ln -sf newRef referenceFile           ##  Bloody Temporary")
                print ("RL=`grep {0} $SST_TEST_REFERENCE/test_SweepEmber.out`".format(hex_dig))
                #print ("diff referenceFile outFile")
                #print ("if [ $? != 0 ] ; then")
                print ("if [[ \"$RL\" != *\"$TL\"* ]] ; then ")
                print ("    echo output does not match reference time")
                print ("    echo Reference entry is $RL")
                print ("    fail \"output does not match reference time\"")
                #print ("    echo Reference File:")
                #print ("    cat referenceFile ")
                print ("    echo Out Put file:")
                print ("    cat outFile ")
                print ("else")
                print ("    echo ' '; echo Test Passed; echo ' ' ")
                print ("fi")
                print ("popd")
                print ("}")
                #call("sst --model-options=\"--topo={0} {1} --cmdLine=\\\"Init\\\" --cmdLine=\\\"{2} {3}\\\"--cmdLine=\\\"Fini\\\"\" {4}".format(network['topo'], x, test['motif'], y, config), shell=True )
