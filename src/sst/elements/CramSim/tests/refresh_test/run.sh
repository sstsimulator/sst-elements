#!/bin/bash

#tests for ddr4
sst --lib-path=../../.libs/ test_txngen.py --model-options="--txngen=trace --configfile=./ddr4_test.cfg --tracefile=./testSeq.seq00 --stopAtCycle=100us" > log_trace
../../waterfall.py ddr4_trace > wf_trace

sst --lib-path=../../.libs/ test_txngen.py --model-options="--txngen=trace --configfile=./ddr4_openpage_test.cfg --tracefile=./testSeq.seq00 --stopAtCycle=100us" > log_openpage_trace
../../waterfall.py ddr4_trace > wf_trace_openpage
sst --lib-path=../../.libs/ test_txngen.py --model-options="--txngen=rand --configfile=./ddr4.cfg --stopAtCycle=100us" > log_rand
../../waterfall.py ddr4_trace > wf_rand
sst --lib-path=../../.libs/ test_txngen.py --model-options="--txngen=rand --configfile=./ddr4_openpage.cfg --stopAtCycle=100us" > log_openpage_rand
../../waterfall.py ddr4_trace > wf_openpage_rand


#tests for hbm
sst --lib-path=../../.libs/ test_txngen.py --model-options="--txngen=rand --configfile=./hbm_test.cfg --stopAtCycle=100us" > log_hbm_rand
../../waterfall.py hbm_trace > wf_hbm_rand

sst --lib-path=../../.libs/ test_txngen.py --model-options="--txngen=rand --configfile=./hbm_openpage_test.cfg --stopAtCycle=100us" > log_hbm_openpage_rand
../../waterfall.py hbm_trace > wf_hbm_openpage_rand

sst --lib-path=../../.libs/ test_txngen.py --model-options="--txngen=rand --configfile=./hbm_sbr_test.cfg --stopAtCycle=100us" > log_hbm_sbr_rand
../../waterfall.py hbm_trace > wf_hbm_sbr_rand

sst --lib-path=../../.libs/ test_txngen.py --model-options="--txngen=rand --configfile=./hbm_openpage_sbr_test.cfg --stopAtCycle=100us" > log_hbm_openpage_sbr_rand
../../waterfall.py hbm_trace > wf_hbm_openpage_sbr_rand


