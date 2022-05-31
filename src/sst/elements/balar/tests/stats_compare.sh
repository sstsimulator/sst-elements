# Some helpful cmd to comapre stats output from mmio and original balar version

# Get mmio memory system stat
grep -E -e '(l1gcache_.*)|(l2gcache_.*)|(Simplehbm_.*)' stats.out > mmio_mem.out

# Get original memory system stat
grep -E -e '(l1gcache_.*)|(l2gcache_.*)|(Simplehbm_.*)' refFiles/test_gpgpu_vectorAdd.out > prev_mem.out

# Compare
git diff --word-diff=color --no-index mmio_mem.out prev_mem.out > stats_compare.out

# Extract differences not caused by sim time
grep -E -e '.*[^(SimTime)] = .\[31m.*' stats_compare.out > stats_compare_exclude_simtime.out