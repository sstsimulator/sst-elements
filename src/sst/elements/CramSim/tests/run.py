import os
import time

#sst --lib-path=../.libs/ test_txntrace4.py --model-options="--configfile=../hbm_legacy_8h.cfg --tracefile=../traces/seq_hbm.seq02" 
#sst --lib-path=../.libs/ test_txntrace4.py --model-options="--configfile=../hbm_pseudo_8h.cfg --tracefile=../traces/seq_hbm.seq02" 
#sst --lib-path=../.libs/ test_newarch.py --model-options="--configfile=../hbm_pseudo_1h.cfg --tracefile=../traces/seq_hbm.seq02" 
#sst --lib-path=../.libs/ test_txngenrand1.py --model-options="--configfile=../hbm_pseudo_1h.cfg" 
#sst --lib-path=../.libs/ test_txngenrand1.py --model-options="--configfile=../ddr4_verimem.cfg" 
#sst --lib-path=../.libs/ test_newarch.py --model-options="--configfile=../hbm_legacy_8h.cfg --tracefile=../traces/seq_hbm.seq02" 
#sst --lib-path=../.libs/ test_statTest1.py --model-options="--configfile=../ddr4_verimem.cfg --tracefile=../traces/sst-CramSim-trace_verimem_6_W.trc"
#sst --lib-path=../.libs/ test_statTest1.py --model-options="--configfile=../ddr4_verimem.cfg --tracefile=../traces/sst-CramSim-trace_verimem_6_W.trc"

sstCmd = "sst --lib-path=.libs/ ./tests/test_txngen.py --model-options=\""

#seq txn gen
#sstParams = "--configfile=../ddr4_verimem.cfg --txngen=seq\""

#seq txn gen
#sstParams = "--configfile=../ddr4_verimem.cfg --txngen=rand\""

#trace
sstParams = "--configfile=../ddr4_verimem.cfg --txngen=trace --tracefile=../traces/sst-CramSim-trace_verimem_6_W.trc --stopAtCycle=1ms\""

def run(sstParams):
	# run sst
	osCmd = sstCmd + sstParams
	print osCmd

	start=time.time()
	os.system(osCmd)
	end=time.time()
	sim_elapsed_time=end-start
	print "sim_elapsed_time(sec): ", sim_elapsed_time
	print "\n\n"

stopAtCycle = "100us"
sstParams = [
	"--configfile=./ddr4_verimem.cfg --txngen=rand --stopAtCycle=%s\"" % stopAtCycle,
	"--configfile=./ddr4_verimem.cfg --txngen=seq  --stopAtCycle=%s\"" % stopAtCycle,
	"--configfile=./ddr4_verimem.cfg --txngen=trace --tracefile=./traces/sst-CramSim-trace_verimem_1_R.trc --stopAtCycle=%s\"" % stopAtCycle,
	"--configfile=./ddr4_verimem.cfg --txngen=trace --tracefile=./traces/sst-CramSim-trace_verimem_2_R.trc --stopAtCycle=%s\"" % stopAtCycle,
	"--configfile=./ddr4_verimem.cfg --txngen=trace --tracefile=./traces/sst-CramSim-trace_verimem_4_R.trc --stopAtCycle=%s\"" % stopAtCycle,
	"--configfile=./ddr4_verimem.cfg --txngen=trace --tracefile=./traces/sst-CramSim-trace_verimem_1_W.trc --stopAtCycle=%s\"" % stopAtCycle,
	"--configfile=./ddr4_verimem.cfg --txngen=trace --tracefile=./traces/sst-CramSim-trace_verimem_2_W.trc --stopAtCycle=%s\"" % stopAtCycle,
	"--configfile=./ddr4_verimem.cfg --txngen=trace --tracefile=./traces/sst-CramSim-trace_verimem_4_W.trc --stopAtCycle=%s\"" % stopAtCycle,
	"--configfile=./ddr4_verimem.cfg --txngen=trace --tracefile=./traces/sst-CramSim-trace_verimem_1_RW.trc --stopAtCycle=%s\"" % stopAtCycle]

for params in sstParams:
	run(params)
	time.sleep(1)
