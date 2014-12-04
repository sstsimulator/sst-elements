#!/bin/sh

for i in sdl*.py.new ; do
    target=`basename $i .new`
    cp $i ${target}.ref
done
