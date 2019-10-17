LOADFILE="qos.load"

export PYTHONPATH="../test"

#../sst  \
/Users/kshemme/work/sst/ember-fix/bin/sst \
--model-options=" \
--loadFile=$LOADFILE \
--platform=default \
--topo=fattree \
--shape=16,16:32 \
" \
../test/emberLoad.py
