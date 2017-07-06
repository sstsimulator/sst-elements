#!/bin/bash

sst --lib-path=../../.libs/ test_txngen.py --model-options="--txngen=trace --configfile=./hbm_ch_test_open.cfg --tracefile=./seq_hbm.seq00 --stopAtCycle=2us" > log_open
../../waterfall.py hbm_trace >wf_open
#sst --lib-path=../../.libs/ test_txngen.py --model-options="--txngen=trace --configfile=./hbm_ch_test.cfg --tracefile=./seq_hbm.seq00 --stopAtCycle=2us" > log_close
#../../waterfall.py hbm_trace >wf_close
#sst --lib-path=../../.libs/ test_txngen.py --model-options="--txngen=trace --configfile=./hbm_ch_test_open_refresh.cfg --tracefile=./seq_hbm.seq00 --stopAtCycle=2us" > log_open_refresh
#../../waterfall.py hbm_trace >wf_open_refresh
