#!/usr/bin/python

import os
import sys

cfgs =[
	"ddr4",
	"ddr4_readfirst",
	"ddr4_readfirst_quickres",
	"ddr4_quickres"
	]

for cfg in cfgs:
	log = "log_%s"%cfg
	trace = "%s_trace"%cfg
	option="--lib-path=../../.libs/ test_txngen.py --model-options=\"--txngen=trace --configfile=./%s.cfg --stopAtCycle=100us --tracefile=cramsim-stream.trace\">%s"%(cfg,log) 

	cmd = "sst "+option
	os.system(cmd)
	os.system("../../waterfall.py %s > wf_%s"%(trace,cfg))
