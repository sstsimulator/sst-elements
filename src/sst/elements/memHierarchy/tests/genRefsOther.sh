#!/bin/bash

echo "Generating reference files..."
# Cassini
echo "Cassini..."
sst ../../cassini/tests/streamcpu-nbp.py > ../../cassini/tests/refFiles/test_cassini_prefetch_nbp.out &
sst ../../cassini/tests/streamcpu-nopf.py > ../../cassini/tests/refFiles/test_cassini_prefetch_nopf.out &
sst ../../cassini/tests/streamcpu-sp.py > ../../cassini/tests/refFiles/test_cassini_prefetch_sp.out &
wait

# Miranda
echo "Miranda..."
sst ../../miranda/tests/copybench.py > ../../miranda/tests/refFiles/test_miranda_copybench.out &
sst ../../miranda/tests/streambench.py > ../../miranda/tests/refFiles/test_miranda_streambench.out &
sst ../../miranda/tests/gupsgen.py > ../../miranda/tests/refFiles/test_miranda_gupsgen.out &
sst ../../miranda/tests/inorderstream.py > ../../miranda/tests/refFiles/test_miranda_inorderstream.out &
sst ../../miranda/tests/randomgen.py > ../../miranda/tests/refFiles/test_miranda_randomgen.out &
sst ../../miranda/tests/revsinglestream.py > ../../miranda/tests/refFiles/test_miranda_revsinglestream.out &
sst ../../miranda/tests/singlestream.py > ../../miranda/tests/refFiles/test_miranda_singlestream.out &
sst ../../miranda/tests/stencil3dbench.py > ../../miranda/tests/refFiles/test_miranda_stencil3dbench.out &
wait

# Ariel
echo "Ariel..."
cd ../../ariel/frontend/simple/examples/stream/
make
export OMP_EXE=./stream
sst ariel_ivb.py > tests/refFiles/test_Ariel_ariel_ivb.out &
sst ariel_snb.py > tests/refFiles/test_Ariel_ariel_snb.out &
sst memHstream.py > tests/refFiles/test_Ariel_memHstream.out &
sst runstreamNB.py > tests/refFiles/test_Ariel_runstreamNB.out &
sst runstream.py > tests/refFiles/test_Ariel_runstream.out &
sst runstreamSt.py > tests/refFiles/test_Ariel_runstreamSt.out &
wait

# cacheTracer
echo "CacheTracer..."
cd ../../../../../cacheTracer/tests
sst test_cacheTracer_1.py > refFiles/test_cacheTracer_1.out &
sst test_cacheTracer_2.py > refFiles/test_cacheTracer_2.out &
wait

# GNA
echo "gensa..."
cd ../../GNA/tests    
sst test.py > test.ref.out

# Messier
echo "messier..."
cd ../../Messier/tests
sst gupsgen_2RANKS.py > refFiles/test_Messier_gupsgen_2RANKS.out &
sst gupsgen_fastNVM.py > refFiles/test_Messier_gupsgen_fastNVM.out &
sst gupsgen.py > refFiles/test_Messier_gupsgen.out &
sst stencil3dbench_messier.py > refFiles/test_Messier_stencil3dbench_messier.out &
sst streambench_messier.py > refFiles/test_Messier_streambench_messier.out &
wait

# Opal
echo "opal..."
cd ../../Opal/tests
cd app
make
cd ..
sst basic_1node_1smp.py > refFiles/test_Opal_basic_1node_1smp.out

# Samba
echo "samba..."
cd ../../Samba/tests
sst gupsgen_mmu_4KB.py > refFiles/test_Samba_gupsgen_mmu_4KB.out &
sst gupsgen_mmu.py > refFiles/test_Samba_gupsgen_mmu.out &
sst gupsgen_mmu_three_levels.py > refFiles/test_Samba_gupsgen_mmu_three_levels.out &
sst stencil3dbench_mmu.py > refFiles/test_Samba_stencil3dbench_mmu.out &
sst streambench_mmu.py > refFiles/test_Samba_streambench_mmu.out &
wait

