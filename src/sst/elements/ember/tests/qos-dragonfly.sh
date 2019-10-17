LOADFILE="qos.load"

export PYTHONPATH="../test"

#../sst  \
/Users/kshemme/work/sst/ember-fix/bin/sst \
--model-options=" \
--loadFile=$LOADFILE \
--platform=default \
--topo=dragonfly \
--shape=8:16:16:4 \
" \
../test/emberLoad.py
