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


