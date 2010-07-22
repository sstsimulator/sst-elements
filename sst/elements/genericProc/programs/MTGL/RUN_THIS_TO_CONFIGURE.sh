#!/bin/sh
cprops=""
qthreads=""
while [ "$1" ] ; do
	case "$1" in
		--with-cprops=*)
			cprops="$1"
			shift
			;;
		--with-qthreads=*)
			qthreads="$1"
			shift
			;;
		--sstenv=*)
			sstenv=${1#--sstenv=}
			cprops="--with-cprops=$sstenv"
			qthreads="--with-qthreads=$sstenv"
			shift
			;;
		*)
			break
			;;
	esac
done
if [ -z "$cprops" -o -z "$qthreads" ] ; then
	echo "Need to specify both --with-cprops=/path/to/sst/cprops and"
	echo "--with-qthreads=/path/to/sst/qthreads. Both of those are"
	echo "probably the same, and if so, you can just use --sstenv=/path/to/sst/env"
	echo
	echo "Thus, an example commandline would be:"
	echo '   ./RUN_THIS_TO_CONFIGURE.sh --sstenv=$HOME/sstenv'
	exit
fi
unset CC CXX
./configure \
	--host=powerpc-sst-darwin8.11.0 \
	$cprops \
	$qthreads \
	"$@"
