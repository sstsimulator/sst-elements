#!/bin/bash
if [[ $1 == "clean" ]] ; then
    rm -f fft_bench
    exit
fi

mpicc -Wall -o fft_bench pfft_drive.c dft.c dft.h timestats_structures.h pfft_r2_dit.c ../stat_p.c -lm
