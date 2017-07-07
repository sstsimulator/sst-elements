#!/bin/bash

sst --lib-path=../../.libs/ test_txngen.py --model-options="--txngen=trace --configfile=./ddr4_test_FRFCFS.cfg --tracefile=./testSeq.seq00 --stopAtCycle=10us" > log_frfcfs_trace
../../waterfall.py ddr4_trace > wf_frfcfs_trace

sst --lib-path=../../.libs/ test_txngen.py --model-options="--txngen=trace --configfile=./ddr4_test_FCFS.cfg --tracefile=./testSeq.seq00 --stopAtCycle=10us" > log_fcfs_trace
../../waterfall.py ddr4_trace > wf_fcfs_trace

sst --lib-path=../../.libs/ test_txngen.py --model-options="--txngen=rand --configfile=./ddr4_test_FRFCFS.cfg --tracefile=./testSeq.seq00 --stopAtCycle=10us" > log_frfcfs_rand
../../waterfall.py ddr4_trace > wf_frfcfs_rand

sst --lib-path=../../.libs/ test_txngen.py --model-options="--txngen=rand --configfile=./ddr4_test_FCFS.cfg --tracefile=./testSeq.seq00 --stopAtCycle=10us" > log_fcfs_rand
../../waterfall.py ddr4_trace > wf_fcfs_rand
