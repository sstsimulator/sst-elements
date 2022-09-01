#!/bin/bash
COMPILE="clang++ -fPIC -std=c++11 -I/Users/jpkenny/install/sst-core-devel/include -I/Users/jpkenny/src/sst-elements-hgmerge/src/sst/elements/mercury -I/Users/jpkenny/install/sst-elements-hgmerge/include/sst/elements/hg -g -O0 -c testme.cc"
echo $COMPILE
$COMPILE
LINK="clang++ -shared -Wl,-undefined -Wl,dynamic_lookup -o testme testme.o"
echo $LINK
$LINK

#clang++ -shared -Wl,-undefined -Wl,dynamic_lookup -L/Users/jpkenny/install/sst-elements-opsys/lib/sst-elements-library -lhg -o testme testme.o
