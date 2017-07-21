#!/usr/bin/python

import os
import sys

cfgs =[
	"ddr4",
	"ddr4_readfirst",
	"ddr4_readfirst_fastwrite",
	"ddr4_fastwrite"
	]

for cfg in cfgs:
	log = "log_%s"%cfg
	trace = "%s_trace"%cfg
	option="--lib-path=../../.libs/ test_txngen.py --model-options=\"--txngen=rand --configfile=./%s.cfg --stopAtCycle=100us --tracefile=cramsim-random.trace\">%s"%(cfg,log) 

	cmd = "sst "+option
	os.system(cmd)
	os.system("../../waterfall.py %s > wf_%s"%(trace,cfg))
