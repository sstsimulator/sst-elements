hostname
date
source /mnt/nokrb/fkaplan3/tools/addsimulator.sh
export SIMOUTPUT=/mnt/nokrb/fkaplan3/SST/git/sst/sst/elements/scheduler/simulations/N1_bisection/N1_alpha2_bisection_simple_simple_iter0/
python run_DetailedNetworkSim.py --emberOut ember.out --alpha 2 --schedPy ./simple_simple_bisection_N1.py
date
