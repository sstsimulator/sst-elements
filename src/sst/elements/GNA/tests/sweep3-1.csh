#!/bin/tcsh

# time sst ./test_2.py -- -n 10000 -c $1 -s $2 -m $3 -a $4 -g $5 -C $6
# ./run.slurm $c $s $m $a $g $C

#cache
foreach C (001 004 01)
    foreach c (16 32)
        foreach a (1 2 4 8 16 32)
            sbatch ./run.slurm $c 64 32 $a 1 $C
        end
    end
end
