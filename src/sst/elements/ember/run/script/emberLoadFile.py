#! /usr/bin/env python

import sys,getopt,pprint

pp = pprint.PrettyPrinter(indent=4)

import topoConfig
import platConfig
import rtrConfig
import jobInfo
import emberLoadBase
import loadUtils
import nicConfig

myOptions = ['jobFile='] 
myOptions += topoConfig.getOptions() 
myOptions += platConfig.getOptions()
myOptions += emberLoadBase.getOptions() 

try:
    opts, args = getopt.getopt( sys.argv[1:], "", myOptions + ['help'] )

except getopt.GetoptError as err:
    print str(err)
    sys.exit(2)

jobFile = None

for o,a in opts:
	if o in ('--jobFile'):
		jobFile = a
	if o in ('--help'):
		sys.exit( 'emberLoadJob: options {0} '.format(myOptions) )

if not jobFile:
	sys.exit('FATAL: must specify --jobFile')

topo, shape = topoConfig.parseOptions(opts)
platParams = platConfig.parseOptions(opts)

jobs = []

for jobId, nidlist, cmds in loadUtils.getWorkListFromFile( jobFile, {} ):
	motifs = []
	for cmd in cmds:
		motif = {}
		motif['cmd'] = cmd
		motifs += [ motif ]  

	numNodes = loadUtils.calcNetMapSize( nidlist )
	job = jobInfo.JobInfoCmd( jobId, numNodes , 1, motifs ) 
	job.setNidList( nidlist )
	jobs += [job]

emberLoadBase.run( opts, platParams, topo, shape, jobs )
