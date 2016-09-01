import sys, getopt, os

import topoConfig
import platConfig
import jobInfo
import emberLoadBase
import rtrConfig

myOptions = jobInfo.getOptions() 
myOptions += topoConfig.getOptions()
myOptions += platConfig.getOptions() 
myOptions += emberLoadBase.getOptions() 
myOptions += ['detailedModel=']
myOptions += ['detailedModelParams=']

try:
	opts, args = getopt.getopt( sys.argv[1:], "", myOptions + ["help"] )

except getopt.GetoptError as err:
    print str(err)
    sys.exit(2)

detailedModel = None
detailedModelParams = None
detailedModelNodes = [0]

for o,a in opts:
	if o in ('--detailedModel'):
		detailedModel = a
	if o in ('--detailedModelParams'):
		detailedModelParams = a
	if o in ('--help'):
		sys.exit( 'emberLoadJob: options {0} '.format(myOptions) )

topo, shape = topoConfig.parseOptions(opts)
params = platConfig.parseOptions(opts)

numNodes, ranksPerNode, motifs, random = jobInfo.parseOptions(opts)

job = jobInfo.JobInfoCmd( 0, numNodes, ranksPerNode, motifs )

if detailedModel and detailedModelParams:
	job.setDetailed( [detailedModel, detailedModelParams, detailedModelNodes ] ) 
	

if random:
	job.setRandom()

emberLoadBase.run( opts, params, topo, shape, [ job ], {} ) 
