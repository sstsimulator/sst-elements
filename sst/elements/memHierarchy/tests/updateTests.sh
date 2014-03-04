#!/bin/sh

for i in sdl*.xml ; do
    target=`basename $i .new`
    cp $i ${target}.ref
done
