#!/bin/sh

for i in sdl*.xml.new ; do
    target=`basename $i .new`
    mv $i ${target}.ref
done
