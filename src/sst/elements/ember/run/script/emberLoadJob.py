import sys,getopt

import rtrConfig
import jobInfo
import emberLoadBase

myOptions = ['jobConfig='] 
myOptions += emberLoadBase.getOptions()

try:
	    opts, args = getopt.getopt( sys.argv[1:], "", myOptions + ["help"] )

except getopt.GetoptError as err:
    print str(err)
    sys.exit(2)

jobFile = None

for o,a in opts:
	if o in ('--jobConfig'):
		jobFile = a
	if o in ('--help'):
		sys.exit( 'emberLoadJob: options {0} '.format(myOptions) )

if not jobFile:
	sys.exit('FATAL: must specify --jobConfig')


jobConfig = None
try:
	jobConfig = __import__( jobFile, fromlist=[''] )
except:
	sys.exit('Failed: could not import sim configuration `{0}`'.format(jobFile) )

topo, shape = jobConfig.getTopo()
platParams = jobConfig.getPlatform()

job = jobInfo.JobInfo( 0, jobConfig.getNumNodes(), jobConfig.getRanksPerNode(),
		jobConfig.genWorkFlow ) 
job.setDetailed( jobConfig.getDetailedModel() )

emberLoadBase.run( opts, platParams, topo, shape, [job], jobConfig.getPerNicParams )
