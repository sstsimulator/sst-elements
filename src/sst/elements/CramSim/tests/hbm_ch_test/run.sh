#!/bin/bash

#sst --lib-path=../../.libs/ ../../tests/test_statTest1.py --model-options="--configfile=hbm_ch_test.cfg --tracefile=./seq_hbm.seq00"
sst --lib-path=../../.libs/ ../../tests/test_statTest1.py --model-options="--configfile=hbm_ch_test_open.cfg --tracefile=./seq_hbm.seq00"
../../waterfall.py hbm_trace >wf
