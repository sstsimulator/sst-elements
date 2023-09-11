#!/bin/bash

make tricount_clean

ARGS="configs/input_$1_$2_$3_$4_$5_$6.txt"

if [ ! -f $ARGS ]; then
  echo "File $ARGS does not exist"
  exit
fi

export VANADIS_EXE=$PWD/tricount_clean.riscv64
export VANADIS_EXE_ARGS=$ARGS
time sst tc_vanadis.py
