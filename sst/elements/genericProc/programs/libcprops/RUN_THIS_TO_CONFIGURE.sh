#!/bin/sh
case "$1" in
	--prefix=*)
	;;
	*)
	echo "Must specify --prefix (trust me, you don't want to install this with your host system libraries)"
	exit
	;;
esac
./configure \
	--host=powerpc-sst-darwin8.11.0 \
	--disable-cpsp \
	--disable-cpsvc \
	--disable-cp-dbms \
	--disable-shared \
	"$@"
