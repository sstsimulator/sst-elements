#!/bin/bash

#sst --lib-path=../../.libs/ test_txngen.py --model-options="--txngen=trace --configfile=./ddr4_test.cfg --tracefile=./testSeq.seq00 --stopAtCycle=10us"
sst --lib-path=../../.libs/ test_txngen.py --model-options="--txngen=trace --configfile=./ddr4_test.cfg --tracefile=./testSeq.seq00 --stopAtCycle=100us" > log_trace
../../waterfall.py ddr4_trace > wf_trace

sst --lib-path=../../.libs/ test_txngen.py --model-options="--txngen=trace --configfile=./ddr4_openpage_test.cfg --tracefile=./testSeq.seq00 --stopAtCycle=100us" > log_trace_openpage
../../waterfall.py ddr4_trace > wf_trace_openpage

sst --lib-path=../../.libs/ test_txngen.py --model-options="--txngen=rand --configfile=./ddr4.cfg --stopAtCycle=100us" > log_rand
../../waterfall.py ddr4_trace > wf_rand

sst --lib-path=../../.libs/ test_txngen.py --model-options="--txngen=rand --configfile=./ddr4_openpage.cfg --stopAtCycle=100us" > log_rand_openpage
../../waterfall.py ddr4_trace > wf_rand_openpage

sst --lib-path=../../.libs/ test_txngen.py --model-options="--txngen=rand --configfile=./hbm_test.cfg --stopAtCycle=100us" > log_hbm_rand
../../waterfall.py hbm_trace > wf_hbm_rand

sst --lib-path=../../.libs/ test_txngen.py --model-options="--txngen=rand --configfile=./hbm_openpage_test.cfg --stopAtCycle=100us" > log_hbm_rand_openpage
../../waterfall.py hbm_trace > wf_hbm_rand_openpage

