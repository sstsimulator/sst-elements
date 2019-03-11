#!/bin/tcsh

# time sst ./test_2.py -- -n 10000 -c $1 -s $2 -m $3 -a $4 -g $5 -C $6
# ./run.slurm $c $s $m $a $g $C


#topology 0
foreach s (32 64 128 256)
    sbatch ./run.slurm 256 $s 32 4 0 01
end
foreach c (16 64 256 1024)
    sbatch ./run.slurm $c 128 32 4 0 01
end

#topology 1,2
foreach g (1 2)
    foreach C (001 01 04 08)  #connectivity
        foreach s (32 64 128 256)
            sbatch ./run.slurm 256 $s 32 4 $g $C
        end
        foreach c (16 64 256 1024)
            sbatch ./run.slurm $c 128 32 4 $g $C
        end
    end
end

#cache
foreach C (001 005 004)
    foreach c (16 64 256 1024)
        foreach a (1 4 8 16 32)
            sbatch ./run.slurm $c 64 32 $a 1 $C
        end
    end
end

# paralle
foreach C (001 005 01 04)
    foreach s (32 64 128 256)
        sbatch ./run.slurm 256 $s 32 4 1 $C
    end
end
