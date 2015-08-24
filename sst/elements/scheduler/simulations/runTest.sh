#!/bin/bash

if [ "x$SST_HOME" == "x" ] ; then
    source ~/SST/set_sst_env.sh
    echo $SST_SRC
fi

REFERENCES=$SST_SRC/sqe/test/testReferenceFiles
TEST_INPUTS=$SST_SRC/sqe/test/testInputFiles
TEST_FOLDER=$HOME/SST/local/test

if [ "$#" -eq 0 ] ; then

    ##### TEST 1 ####################

    TEST_NAME=test_scheduler_0001
    OUTFILE=test.temp

    #run sst
    cd $TEST_FOLDER

    echo "" > "$TEST_NAME.sim.alloc"
    echo "" > "$TEST_NAME.sim.time"

    sst test1.sdl > $OUTFILE

    # combine results to match reference

    tail -n +2 "$TEST_NAME.sim.alloc" >> $OUTFILE
    rm "$TEST_NAME.sim.alloc"
    tail -n +2 "$TEST_NAME.sim.time" >> $OUTFILE
    rm "$TEST_NAME.sim.time"

    # compare with reference - output

    diff -u $REFERENCES/$TEST_NAME.out $OUTFILE > $TEST_NAME.tmp

    if [ "`cat $TEST_NAME.tmp`x" == "x" ]
    then
        echo "Test 1 PASSED"
    else
        echo "Test 1 FAILED"
        exit
    fi

    rm $OUTFILE
    rm $TEST_NAME.tmp
else
    echo "Skipping Test 1"
fi

if [ "$#" -eq 0 ] || [ $1 -lt 3 ] ; then

	##### TEST 2 ####################

	TEST_NAME=test_scheduler_0002
	OUTFILE=test.temp
	CONFIG_PATH=$SST_SRC/sst/elements/scheduler/simulations
	jobLog="${TEST_NAME}_joblog.csv"
	faultLog="${TEST_NAME}_faultlog.csv"
	errorLog="${TEST_NAME}_errorlog.csv"

	#run sst
	cd $TEST_FOLDER

	cp $TEST_INPUTS/${TEST_NAME}_joblist.csv .
	sst --sdl-file=$TEST_NAME.xml > $OUTFILE

	echo "===JOB LOG===" >> $OUTFILE
	cat $jobLog >> $OUTFILE
	rm $jobLog
	echo "===FAULT LOG===" >> $OUTFILE
	cat $faultLog >> $OUTFILE
	rm $faultLog
	echo "===ERROR LOG===" >> $OUTFILE
	cat $errorLog >> $OUTFILE
	rm $errorLog

	rm ${TEST_NAME}_joblist.csv
	rm ${TEST_NAME}_joblist.csv.time

	# compare with reference
	diff -u $REFERENCES/$TEST_NAME.out $OUTFILE > $TEST_NAME.tmp

	if [ "`cat $TEST_NAME.tmp`x" == "x" ]
	then
		echo "Test 2 PASSED"
	else
		echo "Test 2 FAILED"
		exit
	fi

	rm $OUTFILE
	rm $TEST_NAME.tmp
else
    echo "Skipping Test 2"
fi

if [ "$#" -eq 0 ] || [ $1 -lt 4 ] ; then

	##### TEST 3 ####################

	TEST_NAME=test_scheduler_0003
	OUTFILE=test.temp

	#run sst
	cd $TEST_FOLDER

	echo "" > "$TEST_NAME.sim.alloc"
	echo "" > "$TEST_NAME.sim.time"

	cp $TEST_INPUTS/${TEST_NAME}.sim .
	cp $SST_SRC/sst/elements/scheduler/simulations/DMatrix4_5_2 .
    cp ${TEST_INPUTS}/testSdlFiles/sdl-3.py .

	sst sdl-3.py > /dev/null

	# combine results to match reference
	echo "" > $OUTFILE
	tail -n +2 "$TEST_NAME.sim.alloc" >> $OUTFILE
	rm "$TEST_NAME.sim.alloc"
	tail -n +2 "$TEST_NAME.sim.time" >> $OUTFILE
	rm "$TEST_NAME.sim.time"

	# compare with reference

	diff -u $REFERENCES/$TEST_NAME.out $OUTFILE > $TEST_NAME.tmp

	if [ "`cat $TEST_NAME.tmp`x" == "x" ]
	then
		echo "Test 3 PASSED"
	else
		echo "Test 3 FAILED"
		exit
	fi
    
    rm sdl-3.py
    rm DMatrix4_5_2
    rm ${TEST_NAME}.sim

	rm $TEST_NAME.tmp
	rm $OUTFILE

else
    echo "Skipping Test 3"
fi

if [ "$#" -eq 0 ] || [ $1 -lt 5 ] ; then

    ##### TEST 4 ####################

    TEST_NAME=test_scheduler_0004
    OUTFILE=test.temp

    #run sst
    cd $TEST_FOLDER

    echo "" > "test_scheduler_Atlas.sim.alloc"
    echo "" > "test_scheduler_Atlas.sim.time"

    cp $SST_SRC/sst/elements/scheduler/simulations/test_scheduler_Atlas.sim .
    cp $SST_SRC/sst/elements/scheduler/simulations/sphere3.mtx .
    cp $SST_SRC/sst/elements/scheduler/simulations/sphere3_coord.mtx .
    sst $TEST_INPUTS/testSdlFiles/${TEST_NAME}.py > /dev/null
    rm test_scheduler_Atlas.sim
    rm sphere3.mtx
    rm sphere3_coord.mtx

    # combine results to match reference

    echo "" > $OUTFILE
    tail -n +2 "test_scheduler_Atlas.sim.alloc" >> $OUTFILE
    rm "test_scheduler_Atlas.sim.alloc"
    tail -n +2 "test_scheduler_Atlas.sim.time" >> $OUTFILE
    rm "test_scheduler_Atlas.sim.time"

    # compare with reference

    diff -u $REFERENCES/$TEST_NAME.out $OUTFILE > $TEST_NAME.tmp

    if [ "`cat $TEST_NAME.tmp`x" == "x" ]
    then
    	echo "Test 4 PASSED"
    else
    	echo "Test 4 FAILED"
    	exit
    fi

    rm $TEST_NAME.tmp
    rm $OUTFILE
else
    echo "Skipping Test 4"
fi

if [ "$#" -eq 0 ] || [ $1 -lt 6 ] ; then

    ##### TEST 5 ####################

    TEST_NAME=test_scheduler_0005
    OUTFILE=test.temp

    #run sst
    cd $TEST_FOLDER

    echo "" > "test_scheduler_Atlas.sim.alloc"
    echo "" > "test_scheduler_Atlas.sim.time"

    cp $SST_SRC/sst/elements/scheduler/simulations/test_scheduler_Atlas.sim .
    cp $SST_SRC/sst/elements/scheduler/simulations/sphere3.mtx .
    cp $SST_SRC/sst/elements/scheduler/simulations/sphere3_coord.mtx .
    cp $TEST_INPUTS/testSdlFiles/${TEST_NAME}.py .
    
    sst ${TEST_NAME}.py > /dev/null

    # combine results to match reference

    echo "" > $OUTFILE
    tail -n +2 "test_scheduler_Atlas.sim.alloc" >> $OUTFILE
    rm "test_scheduler_Atlas.sim.alloc"
    tail -n +2 "test_scheduler_Atlas.sim.time" >> $OUTFILE
    rm "test_scheduler_Atlas.sim.time"

    # compare with reference

    diff -u $REFERENCES/$TEST_NAME.out $OUTFILE > $TEST_NAME.tmp

    if [ "`cat $TEST_NAME.tmp`x" == "x" ]
    then
        echo "Test 5 PASSED"
    else
        echo "Test 5 FAILED"
        exit
    fi
    
    rm ${TEST_NAME}.py
    rm test_scheduler_Atlas.sim
    rm sphere3.mtx
    rm sphere3_coord.mtx

    rm $TEST_NAME.tmp
    rm $OUTFILE

else
    echo "Skipping Test 5"
fi

if [ "$#" -eq 0 ] || [ $1 -lt 7 ] ; then

    ##### TEST 6 ####################

    TEST_NAME=test_DetailedNetwork
    OUTFILE=test.temp

    #run sst

    cd $TEST_FOLDER

    cp $SST_SRC/sst/elements/scheduler/simulations/${TEST_NAME}.sim .
    cp $SST_SRC/sst/elements/scheduler/simulations/*.phase .

    cp $SST_SRC/sst/elements/scheduler/simulations/emberLoad.py .
    emberpath="/home/fkaplan/SST/scratch/src/sst-simulator/sst/elements/ember/test"
    sed -i "s|PATH|$emberpath|g" emberLoad.py

    cp $SST_SRC/sst/elements/scheduler/simulations/run_DetailedNetworkSim.py .
    cp $SST_SRC/sst/elements/scheduler/simulations/snapshotParser_sched.py .
    cp $SST_SRC/sst/elements/scheduler/simulations/snapshotParser_ember.py .
    cp $SST_SRC/sst/elements/scheduler/simulations/${TEST_NAME}.py .
    #cp $TEST_INPUTS/testSdlFiles/${TEST_NAME}.py .
    
    ./run_DetailedNetworkSim.py --xml ${TEST_NAME}.sim.snapshot.xml --emberOut ember.out --schedPy ${TEST_NAME}.py --sched_parser snapshotParser_sched.py --ember_parser snapshotParser_ember.py > /dev/null

    # combine results to match reference

    tail -n +2 "$TEST_NAME.sim.alloc" >> $OUTFILE
    rm "$TEST_NAME.sim.alloc"
    tail -n +2 "$TEST_NAME.sim.time" >> $OUTFILE
    rm "$TEST_NAME.sim.time"

    # compare with reference

    diff -u $REFERENCES/test_scheduler_DetailedNetwork.out $OUTFILE > $TEST_NAME.tmp

    if [ "`cat $TEST_NAME.tmp`x" == "x" ]
    then
        echo "Test 6 PASSED"
    else
        echo "Test 6 FAILED"
        exit
    fi
    
    rm ${TEST_NAME}.py
    rm ${TEST_NAME}.sim
    rm ${TEST_NAME}.sim.snapshot.xml
    rm *.phase
    rm emberLoad.py
    rm run_DetailedNetworkSim.py
    rm snapshotParser_sched.py
    rm snapshotParser_ember.py
    rm $TEST_NAME.tmp
    rm $OUTFILE
    rm ember.out
    rm loadfile
    rm emberCompleted.txt
    rm emberRunning.txt

else
    echo "Skipping Test 6"
fi

echo "ALL TESTS COMPLETED"
