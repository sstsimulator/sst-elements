#!/usr/bin/python

import os
import sys
import time

channels=[1]
traces = [
	"cramsim-random.trace.8M",
#	"cramsim-stream.trace.1M",
	]

cfgs = [
#	"ddr3_ramulator",
	"ddr4_2400_ramulator",
#	"hbm_ramulator"
#	"hbm_ramulator_1ch",
#	"hbm_ramulator_8ch"
	]
for cfg in cfgs:
	for trace in traces:
		log = "log." + cfg + "." + "rand"
		#sstCmd = "mpirun -np 1 sst --partitioner=linear --output-partition=part --lib-path=.libs/ ./tests/test_multilanes_8lane.py --model-options="
		sstCmd = "mpirun -np 1 sst --partitioner=linear --output-partition=part --lib-path=.libs/ ./tests/test_multilanes_8lane.py --model-options="
		#osCmd = sstCmd + "\"--configfile=%s.cfg --txngen=trace --tracefile=/home/seokin/local/packages/ramulator/%s\"> %s&" % (cfg,trace,log)
		osCmd = sstCmd + "\"--configfile=%s.cfg \"" % (cfg)
		print osCmd
		start = time.time()
		os.system(osCmd);
		print "8lane:%f" % (time.time()-start)

'''
sstCmd = "mpirun -np 2 sst --output-partition=part_lane1 --lib-path=.libs/ ./tests/test_multilanes_2lane.py --model-options="
osCmd = sstCmd + "\"--configfile=ddr4_2400.cfg --txngen=trace --tracefile=/home/seokin/local/packages/ramulator/cramsim-random.trace.1M\""

start = time.time()
os.system(osCmd);
print "2lane:%f" % (time.time()-start)

sstCmd = "mpirun -np 4 sst --output-partition=part_lane1 --lib-path=.libs/ ./tests/test_multilanes_4lane.py --model-options="
osCmd = sstCmd + "\"--configfile=ddr4_2400.cfg --txngen=trace --tracefile=/home/seokin/local/packages/ramulator/cramsim-random.trace.1M\""

start = time.time()
os.system(osCmd);
print "4lane:%f" % (time.time()-start)

sstCmd = "mpirun -np 8 sst --output-partition=part_lane1 --lib-path=.libs/ ./tests/test_multilanes_8lane.py --model-options="
osCmd = sstCmd + "\"--configfile=ddr4_2400.cfg --txngen=trace --tracefile=/home/seokin/local/packages/ramulator/cramsim-random.trace.1M\""

start = time.time()
os.system(osCmd);
print "8lane:%f" % (time.time()-start)
'''
'''
sstCmd = "mpirun -np 8 sst --output-partition=part --partitioner=linear --lib-path=.libs/ ./tests/test_multilanes.py --model-options="
osCmd = sstCmd + "\"--configfile=ddr4_2400.cfg --txngen=trace --tracefile=/home/seokin/local/packages/ramulator/cramsim-random.trace.10000\""

start = time.time()
os.system(osCmd);
print "2lane:%f" % (time.time()-start)
sstCmd = "sst --lib-path=.libs/ ./tests/test_txngen.py --model-options="
osCmd = sstCmd + "\"--configfile=ddr4_2400_8ch.cfg --txngen=trace --tracefile=/home/seokin/local/packages/ramulator/cramsim-random.trace.80000\""

start = time.time()
os.system(osCmd);
print time.time()-start
'''


