#!/bin/bash
if [[ $1 == "clean" ]] ; then
    echo "Cleaning up..."
    rm allreduce_bench alltoall_bench msgrate_bench pingpong_bench
    cd src_ghost_bench
	bash Make.sh clean
    cd -

    cd src_fft_bench
	bash Make.sh
    cd -
    exit
fi

mpic++  -Wall allreduce_bench.cc ../stats.cc stat_p.c ../collective_topology.cc -o allreduce_bench
mpic++  -Wall alltoall_bench.cc ../stats.cc stat_p.c ../collective_topology.cc -o alltoall_bench
mpicc -Wall msgrate_bench.c stat_p.c -o msgrate_bench
mpicc -Wall pingpong_bench.c stat_p.c -o pingpong_bench

cd src_ghost_bench
    bash Make.sh
cd -

cd src_fft_bench
    bash Make.sh
cd -
