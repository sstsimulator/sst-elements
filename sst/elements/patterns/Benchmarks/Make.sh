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

mpic++  -Wall $extra -I.. allreduce_bench.cc ../stats.cc stat_p.c ../collective_topology.cc util.c Collectives/allreduce.cc -o allreduce_bench -lm

mpic++  -Wall $extra -I.. alltoall_bench.cc ../stats.cc stat_p.c ../collective_topology.cc util.c Collectives/alltoall.cc -o alltoall_bench -lm

mpicc -Wall $extra msgrate_bench.c stat_p.c util.c -o msgrate_bench -lm
mpicc -Wall $extra pingpong_bench.c stat_p.c util.c -o pingpong_bench -lm

cd src_ghost_bench
    echo -n "Enter "
    pwd
    bash Make.sh
    echo -n "Back in "
cd -

cd src_fft_bench
    echo -n "Enter "
    pwd
    bash Make.sh
    echo -n "Back in "
cd -
