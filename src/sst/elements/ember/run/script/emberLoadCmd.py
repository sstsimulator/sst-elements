import sys, getopt, os

import topoConfig
import platform
import jobInfo
import emberLoadBase
import rtrConfig

myOptions = jobInfo.getOptions() 
myOptions += topoConfig.getOptions()
myOptions += platform.getOptions() 
myOptions += emberLoadBase.getOptions() 

try:
	opts, args = getopt.getopt( sys.argv[1:], "", myOptions + ["help"] )

except getopt.GetoptError as err:
    print str(err)
    sys.exit(2)

for o,a in opts:
	if o in ('--help'):
		sys.exit( 'emberLoadJob: options {0} '.format(myOptions) )

topo, shape = topoConfig.parseOptions(opts)
params = platform.parseOptions(opts)

numNodes, ranksPerNode, motifs, random = jobInfo.parseOptions(opts)

job = jobInfo.JobInfoCmd( 0, numNodes, ranksPerNode, motifs )
if random:
	job.setRandom()

emberLoadBase.run( opts, params, topo, shape, [ job ] ) 
