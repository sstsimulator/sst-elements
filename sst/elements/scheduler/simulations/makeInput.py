'''
SST scheduler simulation input file generator
Input parameters are given below
Setting a parameter to "default" or "" will select the default option
'''
import os

# Input workload trace path:
traceName = 'test_scheduler_Atlas.sim'

# Output file name:
outFile = 'sstInput.py'

# Machine (cluster) configuration:
# mesh[xdim, ydim, zdim], simple. (default: simple)
machine = 'mesh[5,4,4]'

# Number of machine nodes
# The script calculates the number of nodes if mesh machine is provided.
# any integer. (default: 1)
numberNodes = ''

# Number of cores in each machine node
# any integer. (default: 1)
coresPerNode = '4'

# Scheduler algorithm:
# cons, delayed, easy, elc, pqueue, prioritize. (default: pqueue)
scheduler = 'easy' 

# Fair start time algorithm:
# none, relaxed, strict. (default: none)
FST = ''

# Allocation algorithm:
# bestfit, constraint, energy, firstfit, genalg, granularmbs, hybrid, mbs,
# mc1x1, mm, nearest, octetmbs, oldmc1x1,random, simple, sortedfreelist, 
# nearestamap, spectralamap. (default: simple)
allocator = 'bestfit'

# Task mapping algorithm:
# simple, rcb, random, topo, rcm, nearestamap, spectralamap. (default: simple)
taskMapper = 'default'

# Communication overhead parameters
# a[b,c] (default: none)
timeperdistance = '.001865[.1569,0.0129]'

# Heat distribution matrix (D_matrix) input file
# file path, none. (default: none)
dMatrixFile = 'none'

# Randomization seed for communication time overhead
# none, any integer. (default: none)
randomSeed = ''


'''
Do not modify the script after this point.
'''

import sys

if __name__ == '__main__':
  if outFile == "" or outFile == "default":
    print "Error: There is no default value for outFile"
    sys.exit()
  f = open(outFile,'w')
  
  f.write('# scheduler simulation input file\n')
  f.write('import sst\n')
  f.write('\n')
  f.write('# Define SST core options\n')
  f.write('sst.setProgramOption("run-mode", "both")\n')
  f.write('sst.setProgramOption("partitioner", "self")\n')
  f.write('\n')
  f.write('# Define the simulation components\n')
  f.write('scheduler = sst.Component("myScheduler", \
            "scheduler.schedComponent")\n')
  f.write('scheduler.addParams({\n')
  
  if traceName == "" or traceName == "default":
    print "Error: There is no default value for traceName"
    os.remove(outFile)
    sys.exit()
  f.write('      "traceName" : "' + traceName + '",\n')
  if machine != "" and machine != "default":
    f.write('      "machine" : "' + machine + '",\n')
  if coresPerNode != "":
    f.write('      "coresPerNode" : "' + coresPerNode + '",\n')
  if scheduler != "" and scheduler != "default":
    f.write('      "scheduler" : "' + scheduler + '",\n')
  if FST != "" and FST != "default":
    f.write('      "FST" : "' + FST + '",\n')
  if allocator != "" and allocator != "default":
    f.write('      "allocator" : "' + allocator + '",\n')
  if taskMapper != "" and taskMapper != "default":
    f.write('      "taskMapper" : "' + taskMapper + '",\n')
  if timeperdistance != "" and timeperdistance != "default":
    f.write('      "timeperdistance" : "' + timeperdistance + '",\n')
  if dMatrixFile != "" and dMatrixFile != "default":
    f.write('      "dMatrixFile" : "' + dMatrixFile + '",\n')
  if randomSeed != "" and randomSeed != "default":
    f.write('      "runningTimeSeed" : "' + randomSeed + '",\n')
  f.seek(-2, os.SEEK_END)
  f.truncate()
  f.write('\n})\n')
  f.write('\n')
  
  f.write('# nodes\n')
  if machine.split('[')[0] == 'mesh':
    nums = machine.split('[')[1]
    nums = nums.split(']')[0]
    nums = nums.split(',')
    numberNodes = int(nums[0])*int(nums[1])*int(nums[2])
  for i in range(0, numberNodes):
    f.write('n' + str(i) + ' = sst.Component("n' + str(i) + \
            '", "scheduler.nodeComponent")\n')
    f.write('n' + str(i) + '.addParams({\n')
    f.write('      "nodeNum" : "' + str(i) + '",\n')
    f.write('})\n')
  f.write('\n')
    
  f.write('# define links\n')
  for i in range(0, numberNodes):
    f.write('l' + str(i) + ' = sst.Link("l' + str(i) + '")\n')
    f.write('l' + str(i) + '.connect( (scheduler, "nodeLink' + str(i) + \
            '", "0 ns"), (n' + str(i) + ', "Scheduler", "0 ns") )\n')
  f.write('\n')
    
  f.close()

