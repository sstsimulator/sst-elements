#!/bin/bash

make tricount_clean_with_gen

export VANADIS_EXE=$PWD/tricount_clean_with_gen.riscv64
export VANADIS_EXE_ARGS=$*
time sst tc_vanadis.py
