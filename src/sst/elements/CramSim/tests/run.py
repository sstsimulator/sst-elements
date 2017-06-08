#!/usr/bin/python
import os
import time
import sys


def run(sstParams):

	sstCmd = "sst --lib-path=.libs/ ./tests/test_txngen.py --model-options=\""
	# run sst
	osCmd = sstCmd + sstParams
	print osCmd

	start=time.time()
	os.system(osCmd)
	end=time.time()
	sim_elapsed_time=end-start
	print "sim_elapsed_time(sec): ", sim_elapsed_time
	print "\n\n"



stopAtCycle = "1ms"
sstParams = [
#	"--configfile=./ddr4_verimem.cfg --txngen=rand --stopAtCycle=%s\"" % stopAtCycle,
#	"--configfile=./ddr4_verimem.cfg --txngen=seq  --stopAtCycle=%s\"" % stopAtCycle,
	"--configfile=./ddr4_verimem.cfg --txngen=trace --tracefile=./traces/sst-CramSim-trace_verimem_1_R.trc --stopAtCycle=%s\"" % stopAtCycle,
	"--configfile=./ddr4_verimem.cfg --txngen=trace --tracefile=./traces/sst-CramSim-trace_verimem_2_R.trc --stopAtCycle=%s\"" % stopAtCycle,
	"--configfile=./ddr4_verimem.cfg --txngen=trace --tracefile=./traces/sst-CramSim-trace_verimem_4_R.trc --stopAtCycle=%s\"" % stopAtCycle,
	"--configfile=./ddr4_verimem.cfg --txngen=trace --tracefile=./traces/sst-CramSim-trace_verimem_5_R.trc --stopAtCycle=%s\"" % stopAtCycle,
	"--configfile=./ddr4_verimem.cfg --txngen=trace --tracefile=./traces/sst-CramSim-trace_verimem_6_R.trc --stopAtCycle=%s\"" % stopAtCycle,
	"--configfile=./ddr4_verimem.cfg --txngen=trace --tracefile=./traces/sst-CramSim-trace_verimem_1_W.trc --stopAtCycle=%s\"" % stopAtCycle,
	"--configfile=./ddr4_verimem.cfg --txngen=trace --tracefile=./traces/sst-CramSim-trace_verimem_2_W.trc --stopAtCycle=%s\"" % stopAtCycle,
	"--configfile=./ddr4_verimem.cfg --txngen=trace --tracefile=./traces/sst-CramSim-trace_verimem_4_W.trc --stopAtCycle=%s\"" % stopAtCycle,
	"--configfile=./ddr4_verimem.cfg --txngen=trace --tracefile=./traces/sst-CramSim-trace_verimem_5_W.trc --stopAtCycle=%s\"" % stopAtCycle,
	"--configfile=./ddr4_verimem.cfg --txngen=trace --tracefile=./traces/sst-CramSim-trace_verimem_6_W.trc --stopAtCycle=%s\"" % stopAtCycle,
	"--configfile=./ddr4_verimem.cfg --txngen=trace --tracefile=./traces/sst-CramSim-trace_verimem_1_RW.trc --stopAtCycle=%s\"" % stopAtCycle]

#opt=int(sys.argv[1])
#run(sstParams[opt])
for params in sstParams:
	run(params)
	time.sleep(1)
