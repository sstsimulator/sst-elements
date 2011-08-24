#!/bin/bash
# This script assumes that the input file contains the output of multiple ghost runs.
# Each run should be preceeded by a line like this:
#     "# Running on 512 cores, run 3/7  Series Redsky2D.out"
# Indicating how many cores were used, which run it is (3 out of 7 in the example),
# and the file name to deposit the extracted information.
# The exmaple slurm scripts produce this kind of output.

if [[ $# != 1 ]] ; then
    echo "Usage: $0 file"
    echo "    Where \"file\" contains the output of one or more ghost runs."
    exit
fi
in=$1

# Some functions to extract data
function extract_run ()   {
    grep -as "# Running on " $1 | sed -e s/'# Running on \([0-9]*\) cores, run \([0-9]*\)\/\([0-9]*\) *Series \(.*\)$'/'\2'/
}

function extract_total ()   {
    grep -as "# Running on " $1 | sed -e s/'# Running on \([0-9]*\) cores, run \([0-9]*\)\/\([0-9]*\) *Series \(.*\)$'/'\3'/
}

function extract_series ()   {
    grep -as "# Running on " $1 | sed -e s/'# Running on \([0-9]*\) cores, run \([0-9]*\)\/\([0-9]*\) *Series \(.*\)$'/'\4'/
}

function extract_exec_time ()   {
    grep -as "Time to complete on" $1 | sed -e s/'Time to complete on [0-9]* ranks was \([0-9]*[\.]*[0-9]*\) seconds'/'\1'/
}

function extract_nodes ()   {
    grep -as "We are running on" $1 | sed -e s/'We are running on \([0-9]*\) MPI ranks'/'\1'/
}

function extract_compute_percent ()   {
    grep -as "Compute to communicate ratio was " $1 | sed -e s/'Compute to communicate ratio was [0-9]*[\.]*[0-9]*\/[0-9]*[\.]*[0-9]* = [0-9]*\.[0-9]*:[0-9]* (\([0-9]*\.[0-9]*\)% computation, \([0-9]*\.[0-9]*\)% communication)'/'\1'/
}

function extract_communication_percent ()   {
    grep -as "Compute to communicate ratio was " $1 | sed -e s/'Compute to communicate ratio was [0-9]*[\.]*[0-9]*\/[0-9]*[\.]*[0-9]* = [0-9]*\.[0-9]*:[0-9]* (\([0-9]*\.[0-9]*\)% computation, \([0-9]*\.[0-9]*\)% communication)'/'\2'/
}

function extract_compute ()   {
    grep -as "Compute to communicate ratio was " $1 | sed -e s/'Compute to communicate ratio was \([0-9]*[\.]*[0-9]*\)\/\([0-9]*[\.]*[0-9]*\) = [0-9]*\.[0-9]*:[0-9]* (\([0-9]*\.[0-9]*\)% computation, \([0-9]*\.[0-9]*\)% communication)'/'\1'/
}

function extract_communication ()   {
    grep -as "Compute to communicate ratio was " $1 | sed -e s/'Compute to communicate ratio was \([0-9]*[\.]*[0-9]*\)\/\([0-9]*[\.]*[0-9]*\) = [0-9]*\.[0-9]*:[0-9]* (\([0-9]*\.[0-9]*\)% computation, \([0-9]*\.[0-9]*\)% communication)'/'\2'/
}

function extract_bytes_sent_total ()   {
    grep -as "Bytes sent from each rank: " $1 | sed -e s/'Bytes sent from each rank: \([0-9]*\) bytes ([0-9]*\.[0-9]* MB), \([0-9]*\.[0-9]*\) MB total'/'\2'/
}

function extract_num_sends_total ()   {
    grep -as "Sends per rank: " $1 | sed -e s/'Sends per rank: \([0-9]*\), \([0-9]*\) total'/'\2'/
}

function extract_size_per_send ()   {
    grep -as "Average size per send: " $1 | sed -e s/'Average size per send: \([0-9]*\) B (\([0-9]*\.[0-9]*\) MB)'/'\2'/
}

function extract_total_flop ()   {
    grep -as "Total number of flotaing point ops" $1 | sed -e s/'Total number of flotaing point ops \([0-9]*\) (\([0-9]*\.[0-9]*\) x 10^9)'/'\2'/
}

function extract_total_flops ()   {
    grep -as "Per second: " $1 | sed -e s/'Per second: \([0-9]*\) Flops (\([0-9]*\.[0-9]*\) GFlops)'/'\2'/
}

function extract_flops_per_byte ()   {
    grep -as "Flops per byte sent: " $1 | sed -e s/'Flops per byte sent: \([0-9]*\.[0-9]*\) Flops'/'\1'/
}

function extract_border_ratio ()   {
    grep -as "Border to area ratio is" $1 | sed -e s/'Border to area ratio is \([0-9]*\.[0-9]*\)'/'\1'/
}

function extract_grid_x ()   {
    grep -qas "Decomposing data into a 3-D" $1
    if [[ $? == 0 ]] ; then
	# This is a 3-D run
	grep -as "Decomposing data into a 3-D" $1 | sed -e s/'Decomposing data into a 3-D \([0-9]*\) x \([0-9]*\) x \([0-9]*\) block = \([0-9]*\) blocks.*'/'\1'/
    fi
    grep -qas "Decomposing data into a 2-D" $1
    if [[ $? == 0 ]] ; then
	# This is a 2-D run
	grep -as "Decomposing data into a 2-D" $1 | sed -e s/'Decomposing data into a 2-D \([0-9]*\) x \([0-9]*\) grid = \([0-9]*\) rectangles.*'/'\1'/
    fi
}

function extract_grid_y ()   {
    grep -qas "Decomposing data into a 3-D" $1
    if [[ $? == 0 ]] ; then
	# This is a 3-D run
	grep -as "Decomposing data into a 3-D" $1 | sed -e s/'Decomposing data into a 3-D \([0-9]*\) x \([0-9]*\) x \([0-9]*\) block = \([0-9]*\) blocks.*'/'\2'/
    fi
    grep -qas "Decomposing data into a 2-D" $1
    if [[ $? == 0 ]] ; then
	# This is a 2-D run
	grep -as "Decomposing data into a 2-D" $1 | sed -e s/'Decomposing data into a 2-D \([0-9]*\) x \([0-9]*\) grid = \([0-9]*\) rectangles.*'/'\2'/
    fi
}

function extract_grid_z ()   {
    grep -qas "Decomposing data into a 3-D" $1
    if [[ $? == 0 ]] ; then
	# This is a 3-D run
	grep -as "Decomposing data into a 3-D" $1 | sed -e s/'Decomposing data into a 3-D \([0-9]*\) x \([0-9]*\) x \([0-9]*\) block = \([0-9]*\) blocks.*'/'\1'/
    fi
    grep -qas "Decomposing data into a 2-D" $1
    if [[ $? == 0 ]] ; then
	# This is a 2-D run
	echo "1"
    fi
}


#
# Main
#


# Find out how many output records are in the file
num_records=`grep -c "Ghost cell exchange benchmark. Version " $in`
echo "Found $num_records records in $in"

# Find out how many output files there will be
out_file_names=`grep -as "# Running on " $in | sed -e s/'# Running on \([0-9]*\) cores, run \([0-9]*\)\/\([0-9]*\) *Series \(.*\)$'/'\4'/ | sort -u`
echo -e "Will create these output file(s):\n$out_file_names"

for i in $out_file_names ; do
    cat /dev/null > $i
    echo "# Num   | Exec time                 | Compute                   | Communicate               | Total MB | Total  | Avg MB   | Gflop | Gflops                    | Flops per | Border | Grid" >> $i
    echo "# nodes | min, avg, max, median, sd | min, avg, max, median, sd | min, avg, max, median, sd | sent     | sends  | per send | total | min, avg, max, median, sd | byte sent | ratio  | x y z" >> $i
done

let cnt=1
while [[ $cnt -le $num_records ]] ; do
    grep -m $cnt -A 21 "# Running on " $in  | tail -n 22 > rrr.$$

    # Which run is this?
    run=`extract_run rrr.$$`
    total=`extract_total rrr.$$`
    series=`extract_series rrr.$$`
    nodes=`extract_nodes rrr.$$`
    # echo "Run is $run, of $total in $series"

    exec_time[10#$run]=`extract_exec_time rrr.$$`
    exec_comp[10#$run]=`extract_compute rrr.$$`
    exec_comm[10#$run]=`extract_communication rrr.$$`
    flops[10#$run]=`extract_total_flops rrr.$$`

    # These are the same for each run
    bytes_sent=`extract_bytes_sent_total rrr.$$`
    num_sends=`extract_num_sends_total rrr.$$`
    size_per_send=`extract_size_per_send rrr.$$`
    total_flop=`extract_total_flop rrr.$$`
    flops_per_byte=`extract_flops_per_byte rrr.$$`
    border_ratio=`extract_border_ratio rrr.$$`
    grid_x=`extract_grid_x rrr.$$`
    grid_y=`extract_grid_y rrr.$$`
    grid_z=`extract_grid_z rrr.$$`

    if [[ $run -eq $total ]] ; then
	# end of a set of runs
	let i=1
	exec_time_summary=`
	while [[ $i -le $total ]] ; do
	    echo ${exec_time[10#$i]}
	    let i=$i+1
	done | sort -n | ./minavgmax`
	# min, avg, max, median, sd


	let i=1
	compute_time_summary=`
	while [[ $i -le $total ]] ; do
	    echo ${exec_comp[10#$i]}
	    let i=$i+1
	done | sort -n | ./minavgmax`


	let i=1
	comm_time_summary=`
	while [[ $i -le $total ]] ; do
	    echo ${exec_comm[10#$i]}
	    let i=$i+1
	done | sort -n | ./minavgmax`


	let i=1
	total_flops_summary=`
	while [[ $i -le $total ]] ; do
	    echo ${flops[10#$i]}
	    let i=$i+1
	done | sort -n | ./minavgmax`


	# Now store this in the appropriate data file
	echo -en "$nodes\t$exec_time_summary" >> $series
	echo -en "\t$compute_time_summary" >> $series
	echo -en "\t$comm_time_summary" >> $series
	echo -en "\t$bytes_sent" >> $series
	echo -en "\t$num_sends" >> $series
	echo -en "\t$size_per_send" >> $series
	echo -en "\t$total_flop" >> $series
	echo -en "\t$total_flops_summary" >> $series
	echo -en "\t$flops_per_byte" >> $series
	echo -en "\t$border_ratio" >> $series
	echo -en "\t$grid_x" >> $series
	echo -en "\t$grid_y" >> $series
	echo -e "\t$grid_z" >> $series
    fi

    let cnt=$cnt+1
done

let cnt=$cnt-1
echo "Processed $cnt records"
rm rrr.$$
