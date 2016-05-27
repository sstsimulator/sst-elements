hostname
source /mnt/nokrb/fkaplan3/tools/addsimulator.sh
export SIMOUTPUT=/mnt/nokrb/fkaplan3/SST/git/sst/sst/elements/scheduler/simulations/steady_state_analysis/N1_alpha4_alltoall_simple_libtopomap_iter0_motifiter100/
python run_DetailedNetworkSim.py --emberOut ember.out --alpha 4 --schedPy ./simple_libtopomap_alltoall_N1_100iter.py 