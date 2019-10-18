LOADFILE="qos.load"

export PYTHONPATH="../test"

#../sst  \
/Users/kshemme/work/sst/ember-fix/bin/sst \
--model-options=" \
--loadFile=$LOADFILE \
--platform=default \
--topo=hyperx \
--shape=8x8 \
--hostsPerRtr=8 \
" \
../test/emberLoad.py
