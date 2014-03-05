#!/bin/sh

for i in sdl*.xml.new ; do
    target=`basename $i .new`
    cp $i ${target}.ref
done
