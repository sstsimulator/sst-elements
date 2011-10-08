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

if [[ $1 == "extra" ]] ; then
    extra="-Wextra -Wunused-macros -pedantic"
else
    extra=""
fi

mpic++  -Wall $extra allreduce_bench.cc ../stats.cc stat_p.c ../collective_topology.cc util.c -o allreduce_bench -lm
mpic++  -Wall $extra alltoall_bench.cc ../stats.cc stat_p.c ../collective_topology.cc util.c -o alltoall_bench -lm
mpicc -Wall $extra msgrate_bench.c stat_p.c util.c -o msgrate_bench -lm
mpicc -Wall $extra pingpong_bench.c stat_p.c util.c -o pingpong_bench -lm

cd src_ghost_bench
    bash Make.sh
cd -

cd src_fft_bench
    bash Make.sh
cd -
