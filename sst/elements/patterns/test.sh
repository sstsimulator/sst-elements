#!/bin/bash
#
PREFIX=$HOME/work/SST

function genP ()
{
    if [[ $# != 6 ]] ; then
	echo "genP() Needs 6 parameters!"
	exit
    fi

    X=$1; Y=$2; x=$3; y=$4; c=$5; p=$6
    echo "*****************************************************************************************"
    echo "* Generating $p"_"$X"x"$Y"x"$x"x"$y"_"$c.xml"
    echo "*"
    ./genPatterns -s $p"_"$X"x"$Y"x"$x"x"$y"_"$c.xml -X $X -Y $Y -x $x -y $y -c $c -p $p

}

function runP ()
{
    if [[ $# != 6 ]] ; then
	echo "genP() Needs 6 parameters!"
	exit
    fi

    X=$1; Y=$2; x=$3; y=$4; c=$5; p=$6
    f=$p"_"$X"x"$Y"x"$x"x"$y"_"$c.xml
    echo "*****************************************************************************************"
    echo "* Running $f"
    echo "*"
    mpiexec -n 1 $PREFIX/bin/sst.x --sdl-file $f
}



if [[ $# > 0 ]] ; then
    echo
    echo
    echo
    echo "*****************************************************************************************"
    echo "Skipping XML file generation. Hoping they already exist..."
    echo "*****************************************************************************************"
    echo
    echo
    echo
else
    genP 1 1 2 2 1 "ghost"
    genP 2 2 2 2 1 "ghost"
    genP 4 2 2 2 2 "ghost"
    genP 4 2 2 2 8 "ghost"
    genP 4 2 1 1 4 "ghost"
    genP 16 4 1 1 1 "ghost"
    genP 16 4 8 8 16 "ghost"
fi

runP 1 1 2 2 1 "ghost"
runP 2 2 2 2 1 "ghost"
runP 4 2 2 2 2 "ghost"
runP 4 2 2 2 8 "ghost"
runP 4 2 1 1 4 "ghost"		# fails with a huge seg fault in Ghost_pattern::handle_net_events()
runP 16 4 1 1 1 "ghost"

echo
echo
echo
echo "*****************************************************************************************"
echo " The next test will need more than 2 GB of memory!!!!!!"
echo "*****************************************************************************************"
echo
echo
echo
runP 16 4 8 8 16 "ghost"	# Needs more than 2 GB RAM!

