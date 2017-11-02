#!/usr/bin/python
import os
import time
import sys


def run(osCmd):

	print osCmd

	start=time.time()
	os.system(osCmd)
	end=time.time()
	sim_elapsed_time=end-start
	print "sim_elapsed_time(sec): ", sim_elapsed_time
	print "\n\n"


stopAtCycle = "100us"
sysconfig = "../ddr4_verimem.cfg"
sel = 0
if len(sys.argv) > 2:
   sysconfig = sys.argv[2]

if len(sys.argv) > 1:
   sel = sys.argv[1]

sstCmds = {
	1:"sst --lib-path=../.libs ./test_txngen.py --model-options=\"--configfile=%s mode=rand stopAtCycle=%s\"" % (sysconfig,stopAtCycle), 							# test with random trace
	2:"sst --lib-path=../.libs ./test_txngen.py --model-options=\"--configfile=%s mode=seq stopAtCycle=%s\"" % (sysconfig,stopAtCycle),							# test with seq trace
	3:"sst --lib-path=../.libs ./test_txntrace.py --model-options=\"--configfile=%s traceFile=../traces/sst-CramSim-trace_verimem_1_R.trc stopAtCycle=%s\"" % (sysconfig,stopAtCycle),	# test with verimem_1_R trace
	4:"sst --lib-path=../.libs ./test_txntrace.py --model-options=\"--configfile=%s traceFile=../traces/sst-CramSim-trace_verimem_2_R.trc stopAtCycle=%s\"" % (sysconfig,stopAtCycle),	# test with verimem_2_R trace
	5:"sst --lib-path=../.libs ./test_txntrace.py --model-options=\"--configfile=%s traceFile=../traces/sst-CramSim-trace_verimem_4_R.trc stopAtCycle=%s\"" % (sysconfig,stopAtCycle),	# test with verimem_4_R trace
	6:"sst --lib-path=../.libs ./test_txntrace.py --model-options=\"--configfile=%s traceFile=../traces/sst-CramSim-trace_verimem_5_R.trc stopAtCycle=%s\"" % (sysconfig,stopAtCycle),	# test with verimem_5_R trace
	7:"sst --lib-path=../.libs ./test_txntrace.py --model-options=\"--configfile=%s traceFile=../traces/sst-CramSim-trace_verimem_6_R.trc stopAtCycle=%s\"" % (sysconfig,stopAtCycle),	# test with verimem_6_R trace
	8:"sst --lib-path=../.libs ./test_txntrace.py --model-options=\"--configfile=%s traceFile=../traces/sst-CramSim-trace_verimem_1_W.trc stopAtCycle=%s\"" % (sysconfig,stopAtCycle),	# test with verimem_1_W trace
	9:"sst --lib-path=../.libs ./test_txntrace.py --model-options=\"--configfile=%s traceFile=../traces/sst-CramSim-trace_verimem_2_W.trc stopAtCycle=%s\"" % (sysconfig,stopAtCycle),	# test with verimem_2_W trace
	10:"sst --lib-path=../.libs ./test_txntrace.py --model-options=\"--configfile=%s traceFile=../traces/sst-CramSim-trace_verimem_4_W.trc stopAtCycle=%s\"" % (sysconfig,stopAtCycle),	# test with verimem_4_W trace
	11:"sst --lib-path=../.libs ./test_txntrace.py --model-options=\"--configfile=%s traceFile=../traces/sst-CramSim-trace_verimem_5_W.trc stopAtCycle=%s\"" % (sysconfig,stopAtCycle),	# test with verimem_5_W trace
	12:"sst --lib-path=../.libs ./test_txntrace.py --model-options=\"--configfile=%s traceFile=../traces/sst-CramSim-trace_verimem_6_W.trc stopAtCycle=%s\"" % (sysconfig,stopAtCycle),	# test with verimem_6_W trace
	13:"sst --lib-path=../.libs ./test_txntrace.py --model-options=\"--configfile=%s traceFile=../traces/sst-CramSim-trace_verimem_1_RW.trc stopAtCycle=%s\"" % (sysconfig,stopAtCycle),	# test with verimem_1_RW trace
	14:"sst --lib-path=../.libs ./test_txntrace.py --model-options=\"--configfile=%s traceFile=../traces/usimm.trc stopAtCycle=%s traceFileType=USIMM\"" % (sysconfig,stopAtCycle),		# test with usimm trace
	15:"sst --lib-path=../.libs ./test_multilanes_4lane.py --model-options=\"--configfile=../ddr4_2400.cfg stopAtCycle=%s\"" % (stopAtCycle),						# test for txn dispatcher with 4lane
	16:"mpirun -np 5 sst --lib-path=../.libs ./test_multilanes_4lane.py --model-options=\"--configfile=../ddr4_2400.cfg stopAtCycle=%s\"" % (stopAtCycle),			            	# test for txn dispatcher with 4lane(mpi)
	17:"sst --lib-path=../.libs ./test_txngen.py --model-options=\"--configfile=%s mode=seq stopAtCycle=%s\"" % ("../hbm_legacy_4h.cfg",stopAtCycle), 					# test for hbm
	18:"sst --lib-path=../.libs ./test_txngen.py --model-options=\"--configfile=%s mode=rand stopAtCycle=%s\"" % ("../ddr3_power.cfg",stopAtCycle), 							# test for ddr3 with power model
	}                                                                                                                                                                                       

	

if(sel == "batch"):
	for sel in sstCmds:
		run(sstCmds[sel])
		time.sleep(1)
elif(sel.isdigit()):
	params=sstCmds[int(sel)]
	run(params)
else:
	print "Param#1 should be \"batch\" or [0-%d]" % (len(sstCmds)-1)
