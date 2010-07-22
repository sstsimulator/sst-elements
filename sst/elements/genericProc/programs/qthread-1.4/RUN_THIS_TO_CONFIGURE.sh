#!/bin/sh
cprops=""
prefix=""
includes=""
while [ "$1" ] ; do
	case "$1" in
		--with-cprops=*)
			cprops="$1"
			shift
			;;
		--prefix=*)
			prefix="$1"
			shift
			;;
		--sstenv=*)
			sstenv=${1#--sstenv=}
			cprops="--with-cprops=$sstenv"
			prefix="--prefix=$sstenv"
			shift
			;;
		--gproc=*)
			gproc=${1#--gproc=}
			includes="-I$gproc/FE -I$gproc/programs"
			CPPFLAGS="$CPPFLAGS $includes"
			shift
			;;
		*)
			break
			;;
	esac
done
if [ -z "$cprops" -o -z "$prefix" -o -z "$includes" ] ; then
	echo "Need to specify both --with-cprops=/path/to/sst/cprops and"
	echo "--prefix=/path/to/sst/prefix (both of those paths are probably the"
	echo "same). You can use --sstenv=/path/to/sst/env if they are the same."
	echo
	echo "You also need to specify --gproc=/path/to/genericProc so that the"
	echo "necessary CPPFLAGS environment variable can be generated."
	echo
	echo "Thus, an example commandline would be:"
	echo '   ./RUN_THIS_TO_CONFIGURE.sh --sstenv=$HOME/sstenv --gproc=$HOME/sst.svn/sst/elements/genericProc'
	exit
fi
./configure \
	--host=powerpc-sst-darwin8.11.0 \
	--disable-shared \
	"$prefix" \
	"$cprops" \
	"$@"
