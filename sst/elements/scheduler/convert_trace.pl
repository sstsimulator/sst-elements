#!/usr/bin/perl -w

#converts trace from standard workload format into format for compToOpt

#optional command line argument is factor by which to scale arrival times

#Rounding function, based on code from the PERL faq.
sub round
{
    my $num = shift(@_);
    return int($num + .5 * ($num <=> 0));
}

$scaling_factor = 1;
if($#ARGV == 0) {
    $scaling_factor = $ARGV[0];
}

$num_lines = 0;

while($line = <STDIN>) {
    next if($line =~ /^;/);              #skip comments
    next if($line =~ /^\s*$/);           #skip blank lines
    chop $line;
    @fields = split /\s+/, $line;
    shift @fields if($fields[0] eq '');
    if($fields[4] <= 0) {
	print STDERR "skipping job with number of processors=$fields[4]\n";
	next;
    }

    $fields[1] *= $scaling_factor;  #scale the arrival time
    $runTime = $fields[3];
    $runTime = $fields[5] if($runTime == -1);
    $status = $fields[10];

    if(($status != -1) && ($status != 0) && ($status != 1) &&
       ($status != 5 || $runTime <= 0)) {
	print STDERR "skipping job with status $status\n";
	next;
    }
    #print "$fields[1]\t$fields[4]\t$runTime\n";  #w/o estimates
#Output rounded data, in the future this may need to be changed.
    print round $fields[1];
    print "\t";
    print round $fields[4];
    print "\t";
    print round $runTime;
    print "\t";
    print round $fields[8];
    print "\n";   #w/ estimates

    $num_lines++;
}

print STDERR "number of jobs: $num_lines\n";
