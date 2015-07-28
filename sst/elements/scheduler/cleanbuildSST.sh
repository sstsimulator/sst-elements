#!/bin/bash 

if [ "x$SST_SRC" == "x" ] ; then
    source ~/SST/set_sst_env.sh
    echo $SST_SRC
fi

if [[ $# -ne 0 && "$1" == "complete" ]]
  then
    cd ${SST_SRC}
    make all -j 16
    make install

elif [[ $# -ne 0 && "$1" == "all" ]]
  then
    cd ${SST_SRC}
    make all -j 16

elif [[ $# -ne 0 && "$1" == "ins" ]]
  then
    cd ${SST_SRC}
    make install

elif [[ $# -ne 0 && "$1" == "partial" ]]
  then
    cd $SST_SRC
    make distclean
    
    # ignore unused elements
    echo "" > sst/elements/ariel/.ignore
    echo "" > sst/elements/cassini/.ignore
    echo "" > sst/elements/chdlComponent/.ignore
    #echo "" > sst/elements/ember/.ignore
    echo "" > sst/elements/event_test/.ignore
    #echo "" > sst/elements/firefly/.ignore
    echo "" > sst/elements/gem5/.ignore
    #echo "" > sst/elements/hermes/.ignore
    echo "" > sst/elements/iris/.ignore
    echo "" > sst/elements/m5C/.ignore          
    echo "" > sst/elements/memHierarchy/.ignore
    echo "" > sst/elements/miranda/.ignore
    echo "" > sst/elements/mosaic/.ignore
    echo "" > sst/elements/odin/.ignore
    #echo "" > sst/elements/patterns/.ignore
    echo "" > sst/elements/portals/.ignore
    echo "" > sst/elements/portals4_sm/.ignore
    echo "" > sst/elements/prospero/.ignore
    echo "" > sst/elements/qsimComponent/.ignore
    echo "" > sst/elements/savannah/.ignore
    echo "" > sst/elements/SS_router/.ignore
    echo "" > sst/elements/sst_mcniagara/.ignore
    echo "" > sst/elements/sst_mcopteron/.ignore
    echo "" > sst/elements/SysC/.ignore
    #echo "" > sst/elements/vanadis/.ignore
    echo "" > sst/elements/VaultSimC/.ignore
    echo "" > sst/elements/zodiac/.ignore

    ./autogen.sh
    ./configure --prefix=$SST_HOME --with-boost=$BOOST_HOME --with-glpk=$GLPK_HOME --with-metis=$METIS_HOME
    make all -j 16
    make install

elif [[ $# -ne 0 && "$1" == "full" ]]
  then
    cd $SST_SRC
    make distclean
    
    rm sst/elements/ariel/.ignore
    rm sst/elements/cassini/.ignore
    rm sst/elements/chdlComponent/.ignore
    rm sst/elements/ember/.ignore
    rm sst/elements/event_test/.ignore
    rm sst/elements/firefly/.ignore
    rm sst/elements/gem5/.ignore
    rm sst/elements/hermes/.ignore
    rm sst/elements/iris/.ignore
    rm sst/elements/m5C/.ignore          
    rm sst/elements/memHierarchy/.ignore
    rm sst/elements/miranda/.ignore
    rm sst/elements/mosaic/.ignore
    rm sst/elements/odin/.ignore
    rm sst/elements/patterns/.ignore
    rm sst/elements/portals/.ignore
    rm sst/elements/portals4_sm/.ignore
    rm sst/elements/prospero/.ignore
    rm sst/elements/qsimComponent/.ignore
    rm sst/elements/scheduler/.ignore
    rm sst/elements/savannah/.ignore
    rm sst/elements/SS_router/.ignore
    rm sst/elements/sst_mcniagara/.ignore
    rm sst/elements/sst_mcopteron/.ignore
    rm sst/elements/SysC/.ignore
    #rm sst/elements/vanadis/.ignore
    rm sst/elements/VaultSimC/.ignore
    rm sst/elements/zodiac/.ignore
    
    ./autogen.sh
    ./configure --prefix=$SST_HOME --with-boost=$BOOST_HOME --with-glpk=$GLPK_HOME --with-metis=$METIS_HOME
    make all -j 16
    make install

elif [[ $# -ne 0 && "$1" == "clean" ]]
  then
    echo "removing old scheduler"
    rm -r $SST_HOME
    #cd $SST_SRC/sst/elements
    #rm -rf scheduler
    #svn update
    cd $SST_SRC
    cd ..
    rm -rf $SST_SRC
    echo "checking out the svn repository..."
    svn checkout https://www.sst-simulator.org/svn/sst/trunk/ sst-simulator

else
    echo "usage:"
    echo " 1- ./cleanbuildSST.sh all - only make all"
    echo " 2- ./cleanbuildSST.sh ins - only make install"
    echo " 3- ./cleanbuildSST.sh complete - make all & install all elements"
    echo " 4- ./cleanbuildSST.sh clean - cleans up everything and checks out the repo"
    echo " 5- ./cleanbuildSST.sh partial - installs selected elements"
    echo " 6- ./cleanbuildSST.sh full - removes \".ignore\"s and installs all elements"
fi
