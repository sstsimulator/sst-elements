#!/usr/bin/perl

use warnings;

use File::Compare;


$jobLog = "job.log";
$faultLog = "fault.log";
$errorLog = "error.log";
$testLog = "test.log";
$cleancmd = "rm $jobLog $faultLog $errorLog *.time $testLog 2> /dev/null";
$xmlDir = "xml";
$SSTloc = "../../../core";
$SSTargs = " --verbose --sdl-file=";

open JOBLOG, "traces/10_4_nodes.csv";
1 while <JOBLOG>;
$numJobs = $. - 1;
close JOBLOG;

system( $cleancmd );



# testing with no faults


system( "echo \"testing with $xmlDir/4.nofaults.xml\n\" > $testLog" );
system( "$SSTloc/sst $SSTargs$xmlDir/4.nofaults.xml >> $testLog" );

die "Test 1 failed, fault[s] were generated.  See $testLog\n" if -e $faultLog;
die "Test 1 failed, error[s] were generated.  See $testLog\n" if -e $errorLog;

open JOBLOG, $jobLog or die "Could not open job log: $jobLog";
1 while <JOBLOG>;
die "Test 1 failed, not all jobs ran/completed.  See $testLog\n" if $. != 2*$numJobs + 1;
close JOBLOG;

print "test 1 passed\n";

system( $cleancmd );


# test with fault on one leaf node


system( "echo \"testing with $xmlDir/4.oneleaffault.xml\n\" > $testLog" );
system( "$SSTloc/sst $SSTargs$xmlDir/4.oneleaffault.xml >> $testLog" );

die "Test 2 failed, fault[s] were not generated.  See $testLog\n" unless -s $faultLog;
die "Test 2 failed, error[s] were not generated.  See $testLog\n" unless -s $errorLog;

open JOBLOG, $jobLog or die "Could not open job log: $jobLog";
1 while <JOBLOG>;
die "Test 2 failed, not all jobs ran/completed.  See $testLog\n" if $. != 2*$numJobs + 1;
close JOBLOG;

open FAULTLOG, $faultLog or die "Could not open fault log: $faultLog";
while( <FAULTLOG> ){
  m/[^,]+,([^,]+),[^,]+/;
  die "Test 2 failed, an incorrect node shows faults.  See $testLog\n" if $1 ne "1.1" and $1 ne "host";
}
close FAULTLOG;

open SYMPTOMLOG, $errorLog or die "Could not open error log: $errorLog";
while( <SYMPTOMLOG> ){
  m/[^,]+,([^,]+),([^,]+)/;
  die "Test 2 failed, an incorrect node shows errors.  See $testLog\n" if $1 ne "1.1" and $1 ne "host";
  die "Test 2 failed, a node shows a suppressed error.  See $testLog\n" if $2 eq "fault_b\n" or $2 eq "fault_b";
}
close SYMPTOMLOG;

print "test 2 passed\n";

system( $cleancmd );


# test with hierarcy faults


system( "echo \"testing with $xmlDir/4.oneparentfault.xml\n\" > $testLog" );
system( "$SSTloc/sst $SSTargs$xmlDir/4.oneparentfault.xml >> $testLog" );

die "Test 3 failed, fault[s] were not generated.  See $testLog\n" unless -s $faultLog;
die "Test 3 failed, error[s] were not generated.  See $testLog\n" unless -s $errorLog;

open JOBLOG, $jobLog or die "Could not open the job log: $jobLog";
1 while <JOBLOG>;
die "Test 3 failed, not all jobs ran/completed.  See $testLog\n" if $. != 2*$numJobs + 1;
close JOBLOG;

open FAULTLOG, $faultLog or die "Could not open fault log: $faultLog";
while( <FAULTLOG> ){
  m/[^,]+,([^,]+),[^,]+/;
  die "Test 3 failed, an incorrect node shows faults.  See $testLog\n" if $1 ne "1.1" and $1 ne "1.2" and $1 ne "2.1" and $1 ne "host";
}
close FAULTLOG;

open SYMPTOMLOG, $errorLog or die "Could not open error log: $errorLog";
while( <SYMPTOMLOG> ){
  m/[^,]+,([^,]+),([^,]+)/;
  die "Test 3 failed, a leaf shows supressed errors.  See $testLog\n" if $1 eq "1.1" and ($2 eq "fault_b\n" or $2 eq "fault_b");
  die "Test 3 failed, a parent shows supressed errors.  See $testLog\n" if $1 eq "2.1" and ($2 eq "fault_a\n" or $2 eq "fault_a");
}
close SYMPTOMLOG;

print "test 3 passed\n";

system( $cleancmd );


# test that jobs are killed by a fatal fault - default job kill probability (1)


system( "echo \"testing with $xmlDir/1.leaffault_die_default.xml\n\" > $testLog" );
system( "$SSTloc/sst $SSTargs$xmlDir/1.leaffault_die_default.xml >> $testLog" );

die "Test 4 failed, fault[s] were not generated.  See $testLog\n" unless -s $faultLog;
die "Test 4 failed, error[s] were not generated.  See $testLog\n" unless -s $errorLog;

open JOBLOG, $jobLog or die "Could not open the job log: $jobLog";
while( <JOBLOG> ){
  die "Test 4 failed, a job wasn't killed by a fatal fault.  See $testLog\n" if $_ =~ /10000001/;       # this should only show up in the log if the job ran correctly, but this job error'd
}
die "Test 4 failed, not all jobs ran/completed.  See $testLog\n" if $. != 2 + 1;
close JOBLOG;

print "test 4 passed\n";

system( $cleancmd );


# test that jobs are killed by a fatal fault - probability 1


system( "echo \"testing with $xmlDir/1.leaffault_die_1.xml\n\" > $testLog" );
system( "$SSTloc/sst $SSTargs$xmlDir/1.leaffault_die_1.xml >> $testLog" );

die "Test 5 failed, fault[s] were not generated.  See $testLog\n" unless -s $faultLog;
die "Test 5 failed, error[s] were not generated.  See $testLog\n" unless -s $errorLog;

open JOBLOG, $jobLog or die "Could not open the job log: $jobLog";
while( <JOBLOG> ){
  die "Test 5 failed, a job wasn't killed by a fatal fault.  See $testLog\n" if $_ =~ /10000001/;       # this should only show up in the log if the job ran correctly, but this job error'd
}
die "Test 5 failed, not all jobs ran/completed.  See $testLog\n" if $. != 2 + 1;
close JOBLOG;

print "test 5 passed\n";

system( $cleancmd );


# test that jobs are not killed by nonfatal faults - probability 0


system( "echo \"testing with $xmlDir/1.leaffault_nodie.xml\n\" > $testLog" );
system( "$SSTloc/sst $SSTargs$xmlDir/1.leaffault_nodie.xml >> $testLog" );

die "Test 6 failed, fault[s] were not generated.  See $testLog\n" unless -s $faultLog;
die "Test 6 failed, error[s] were not generated.  See $testLog\n" unless -s $errorLog;

open JOBLOG, $jobLog or die "Could not open the job log: $jobLog";
$successFlag = 0;
while( <JOBLOG> ){
  $successFlag = 1 if $_ =~ /10000001/;       # this should only show up in the log if the job ran correctly
}
die "Test 6 failed, a job was killed by a nonfatal fault.  See $testLog\n" if $successFlag == 0;
die "Test 6 failed, not all jobs ran/completed.  See $testLog\n" if $. != 2 + 1;
close JOBLOG;

print "test 6 passed\n";

system( $cleancmd );


# test that faults are delayed correctly when a delay attribute is set


system( "echo \"testing with $xmlDir/4.oneparentfault.delay.xml\n\" > $testLog" );
system( "$SSTloc/sst $SSTargs$xmlDir/4.oneparentfault.delay.xml >> $testLog" );

die "Test 7 failed, fault[s] were not generated.  See $testLog\n" unless -s $faultLog;
die "Test 7 failed, error[s] were not generated.  See $testLog\n" unless -s $errorLog;

@ErrorTimes = ();
@FaultTimes = ();

open ERRORLOG, $errorLog or die "Could not open the error log: $errorLog";
while( <ERRORLOG> ){
  if( $_ =~ /^\d/ ){
    m/([^,]+),([^,]+),([^,]+)/;
    push( @ErrorTimes, $1 );
  }
}
close ERRORLOG;

open FAULTLOG, $faultLog or die "Could not open the error log: $faultLog";
while( <FAULTLOG> ){
  if( $_ =~ /^\d/ ){
    m/([^,]+),([^,]+),([^,]+)/;

    if( scalar( @FaultTimes ) < scalar( @ErrorTimes ) ){
      push( @FaultTimes, $1 );
    }
  }
}
close FAULTLOG;

@Delays = ();

for $count (0 .. (scalar( @FaultTimes ) - 1)){
  push( @Delays, $ErrorTimes[ $count ] - $FaultTimes[ $count ] );
  if( ($ErrorTimes[ $count ] - $FaultTimes[ $count ]) > 8 ){
    print $Delays[ $count ] . "\n";
  }
}

$totalDelay = 0;

for $count (0 .. (scalar( @Delays ) - 1)){
  $totalDelay += $Delays[ $count ];
}

$avgDelay = $totalDelay / scalar( @Delays );

die "Test 7 failed, faults were not delayed enough.  (lower bound > observed)  (5.9 > $avgDelay)  See $testLog\n" if $avgDelay < 5.9;
die "Test 7 failed, faults were delayed too much.  (upper bound < observed)  (6.1 < $avgDelay)  See $testLog\n" if $avgDelay > 6.1;

# Around 70k to 75k faults should have been generated in the last test, delayed by either 5, 6, or 7 seconds.
# Since these delays are selected completely randomly, we would expect the average to be about 6 seconds.
# An actual test found the delay to be ~6.001034, so these margins should be plenty safe enough.

print "test 7 passed\n";

system( $cleancmd );


# test that a parent kill overrides a child nokill


system( "echo \"testing with $xmlDir/4.oneparentfault.allfaultskill.xml\n\" > $testLog" );
system( "$SSTloc/sst $SSTargs$xmlDir/4.oneparentfault.allfaultskill.xml >> $testLog" );

die "Test 8 failed, fault[s] were not generated.  See $testLog\n" unless -s $faultLog;
die "Test 8 failed, error[s] were not generated.  See $testLog\n" unless -s $errorLog;

open JOBLOG, $jobLog or die "Could not open the job log: $jobLog";
$successFlag = 1;
while( <JOBLOG> ){
  m/([^,]+),([^,]+),([^,]+),([^,]+),[^,]+/;
  $successFlag = 0 if $4 eq "0";       # this should only show up in the log if a job wasn't killed
}
die "Test 8 failed, a scheduled node didn't kill a job when a parent's fault dictated that it should.  See $testLog\n" if $successFlag != 1;
die "Test 8 failed, not all jobs ran/completed.  See $testLog\n" if $. != 20 + 1;
close JOBLOG;

print "test 8 passed\n";

system( $cleancmd );


# test that a parent nokill overrides a child kill


system( "echo \"testing with $xmlDir/4.oneparentfault.parentfaultskill.xml\n\" > $testLog" );
system( "$SSTloc/sst $SSTargs$xmlDir/4.oneparentfault.parentfaultskill.xml >> $testLog" );

die "Test 9 failed, fault[s] were not generated.  See $testLog\n" unless -s $faultLog;
die "Test 9 failed, error[s] were not generated.  See $testLog\n" unless -s $errorLog;

open JOBLOG, $jobLog or die "Could not open the job log: $jobLog";
$successFlag = 1;
while( <JOBLOG> ){
  m/([^,]+),([^,]+),([^,]+),([^,]+),[^,]+/;
  $successFlag = 0 if $4 eq "0";       # this should only show up in the log if a job was killed
}
die "Test 9 failed, a scheduled node overrode a parent's decision to kill a job.  See $testLog\n" if $successFlag != 1;
die "Test 9 failed, not all jobs ran/completed.  See $testLog\n" if $. != 20 + 1;
close JOBLOG;

print "test 9 passed\n";

system( $cleancmd );


# test that errors are generated at correct rates


system( "echo \"testing with $xmlDir/4.root_fault.root_log.xml\n\" > $testLog" );
system( "$SSTloc/sst $SSTargs$xmlDir/4.root_fault.root_log.xml >> $testLog" );

die "Test 10 failed, fault[s] were not generated.  See $testLog\n" unless -s $faultLog;
die "Test 10 failed, error[s] were not generated.  See $testLog\n" unless -s $errorLog;

open ERRORLOG, $errorLog or die "Could not open the error log: $errorLog";
$lastErrorTime = 0;
$errorCount = 0;

while( <ERRORLOG> ){
  m/([^,]+),[^,]+,[^,]+/;
  if( $1 ne "time" ){
    $lastErrorTime = $1;
    ++ $errorCount;

  }
}
close ERRORLOG;

$avgTimeBetweenErrors = $lastErrorTime / $errorCount;

die "Test 10 failed, error rate too high.  (observed $avgTimeBetweenErrors, expected <= 890)  See $testLog\n" if $avgTimeBetweenErrors > 890;
die "Test 10 failed, error rate too low.  (observed $avgTimeBetweenErrors, expected >= 838)  See $testLog\n" if $avgTimeBetweenErrors < 838;
# The actual time was calculated to be around 864.  margin for error is +/- 3%

print "test 10 passed\n";

system( $cleancmd );


# test the seeding


system( "echo \"testing with $xmlDir/4.seed_41.xml\n\" > $testLog" );
system( "$SSTloc/sst $SSTargs$xmlDir/4.seed_41.xml >> $testLog" );
system( "mv error.log error.log.41_1; mv fault.log fault.log.41_1; mv job.log job.log.41_1" );
system( "echo \"testing with $xmlDir/4.seed_41.xml\n\" > $testLog" );
system( "$SSTloc/sst $SSTargs$xmlDir/4.seed_41.xml >> $testLog" );
system( "mv error.log error.log.41_2; mv fault.log fault.log.41_2; mv job.log job.log.41_2" );

die "Test 11.1 failed, two tests with the same seed had different fault logs.  See $testLog\n" if compare( "fault.log.41_1", "fault.log.41_2" ) != 0;
die "Test 11.1 failed, two tests with the same seed had different error logs.  See $testLog\n" if compare( "error.log.41_1", "error.log.41_2" ) != 0;
die "Test 11.1 failed, two tests with the same seed had different job logs.  See $testLog\n" if compare( "job.log.41_1", "job.log.41_2" ) != 0;

print "test 11.1 passed\n";

system( "echo \"testing with $xmlDir/4.seed_42.xml\n\" > $testLog" );
system( "$SSTloc/sst $SSTargs$xmlDir/4.seed_42.xml >> $testLog" );
system( "mv error.log error.log.42_1; mv fault.log fault.log.42_1; mv job.log job.log.42_1" );
system( "echo \"testing with $xmlDir/4.seed_42.xml\n\" > $testLog" );
system( "$SSTloc/sst $SSTargs$xmlDir/4.seed_42.xml >> $testLog" );
system( "mv error.log error.log.42_2; mv fault.log fault.log.42_2; mv job.log job.log.42_2" );

die "Test 11.2 failed, two tests with the same seed had different fault logs.  See $testLog\n" if compare( "fault.log.42_1", "fault.log.42_2" ) != 0;
die "Test 11.2 failed, two tests with the same seed had different error logs.  See $testLog\n" if compare( "error.log.42_1", "error.log.42_2" ) != 0;
die "Test 11.2 failed, two tests with the same seed had different job logs.  See $testLog\n" if compare( "job.log.42_1", "job.log.42_2" ) != 0;

print "test 11.2 passed\n";

die "Test 11.3 failed, two tests with different seeds had the same fault logs.  See $testLog\n" if compare( "fault.log.41_1", "fault.log.42_2" ) == 0;
die "Test 11.3 failed, two tests with different seeds had the same error logs.  See $testLog\n" if compare( "error.log.41_1", "error.log.42_2" ) == 0;
die "Test 11.3 failed, two tests with different seeds had the same job logs.  See $testLog\n" if compare( "job.log.41_1", "job.log.42_2" ) == 0;

print "test 11.3 passed\n";

system( "echo \"testing with $xmlDir/4.seed_no.xml\n\" > $testLog" );
system( "$SSTloc/sst $SSTargs$xmlDir/4.seed_no.xml >> $testLog" );
system( "mv error.log error.log.no_1; mv fault.log fault.log.no_1; mv job.log job.log.no_1" );
sleep 2;        # since this test uses the time to seed, make sure the time changes by at least a second between test
system( "echo \"testing with $xmlDir/4.seed_no.xml\n\" > $testLog" );
system( "$SSTloc/sst $SSTargs$xmlDir/4.seed_no.xml >> $testLog" );
system( "mv error.log error.log.no_2; mv fault.log fault.log.no_2; mv job.log job.log.no_2" );

die "Test 11.4 failed, two tests at different times had the same fault logs.  See $testLog\n" if compare( "fault.log.no_1", "fault.log.no_2" ) == 0;
die "Test 11.4 failed, two tests at different times had the same error logs.  See $testLog\n" if compare( "error.log.no_1", "error.log.no_2" ) == 0;
die "Test 11.4 failed, two tests at different times had the same job logs.  See $testLog\n" if compare( "job.log.no_1", "job.log.no_2" ) == 0;

print "test 11.4 passed\n";

die "Test 11.1 failed, two tests with the same seed had different fault logs.  See $testLog\n" if compare( "fault.log.41_1", "fault.log.no_1" ) == 0;
die "Test 11.1 failed, two tests with the same seed had different error logs.  See $testLog\n" if compare( "error.log.41_1", "error.log.no_1" ) == 0;
die "Test 11.1 failed, two tests with the same seed had different job logs.  See $testLog\n" if compare( "job.log.41_1", "job.log.no_1" ) == 0;

print "test 11.5 passed\n";

die "Test 11.1 failed, two tests with the same seed had different fault logs.  See $testLog\n" if compare( "fault.log.41_1", "fault.log.no_2" ) == 0;
die "Test 11.1 failed, two tests with the same seed had different error logs.  See $testLog\n" if compare( "error.log.41_1", "error.log.no_2" ) == 0;
die "Test 11.1 failed, two tests with the same seed had different job logs.  See $testLog\n" if compare( "job.log.41_1", "job.log.no_2" ) == 0;

print "test 11.6 passed\n";

die "Test 11.1 failed, two tests with the same seed had different fault logs.  See $testLog\n" if compare( "fault.log.42_1", "fault.log.no_1" ) == 0;
die "Test 11.1 failed, two tests with the same seed had different error logs.  See $testLog\n" if compare( "error.log.42_1", "error.log.no_1" ) == 0;
die "Test 11.1 failed, two tests with the same seed had different job logs.  See $testLog\n" if compare( "job.log.42_1", "job.log.no_1" ) == 0;

print "test 11.7 passed\n";

die "Test 11.1 failed, two tests with the same seed had different fault logs.  See $testLog\n" if compare( "fault.log.42_1", "fault.log.no_2" ) == 0;
die "Test 11.1 failed, two tests with the same seed had different error logs.  See $testLog\n" if compare( "error.log.42_1", "error.log.no_2" ) == 0;
die "Test 11.1 failed, two tests with the same seed had different job logs.  See $testLog\n" if compare( "job.log.42_1", "job.log.no_2" ) == 0;

print "test 11.8 passed\n";

system( "rm job.log.no_1 job.log.no_2 fault.log.no_1 fault.log.no_2 job.log.41_1 fault.log.41_1 job.log.42_1 job.log.41_2 job.log.42_2 fault.log.41_2 fault.log.42_1 fault.log.42_2 error.log.no_1 error.log.no_2 error.log.41_1 error.log.41_2 error.log.42_1 error.log.42_2" );
system( $cleancmd );


# test the job kill rates


my @test12XML = ( "4.root_fault.leaf_0.1_job_kill.xml",
                  "4.root_fault.leaf_0.2_job_kill.xml",
                  "4.root_fault.leaf_0.4_job_kill.xml",
                  "4.root_fault.leaf_0.8_job_kill.xml",
                  "4.root_fault.leaf_1_job_kill.xml",
                  "4.root_fault.2_leaves_0.1_job_kill.xml",
                  "4.root_fault.2_leaves_0.2_job_kill.xml",
                  "4.root_fault.2_leaves_0.4_job_kill.xml",
                  "4.root_fault.2_leaves_0.8_job_kill.xml",
                  "4.root_fault.2_leaves_1_job_kill.xml",
                  "4.root_fault.3_leaves_0.1_job_kill.xml",
                  "4.root_fault.3_leaves_0.2_job_kill.xml",
                  "4.root_fault.3_leaves_0.4_job_kill.xml",
                  "4.root_fault.3_leaves_0.8_job_kill.xml",
                  "4.root_fault.3_leaves_1_job_kill.xml",
                  "4.root_fault.4_leaves_0.1_job_kill.xml",
                  "4.root_fault.4_leaves_0.2_job_kill.xml",
                  "4.root_fault.4_leaves_0.4_job_kill.xml",
                  "4.root_fault.4_leaves_0.8_job_kill.xml",
                  "4.root_fault.4_leaves_1_job_kill.xml",
                  "4.root_fault.root_0.1_job_kill.xml",
                  "4.root_fault.root_0.2_job_kill.xml",
                  "4.root_fault.root_0.4_job_kill.xml",
                  "4.root_fault.root_0.8_job_kill.xml",
                  "4.root_fault.root_1_job_kill.xml",
                  "4.root_fault.root_1_leaf_0.1_job_kill.xml",
                  "4.root_fault.root_2_leaves_0.1_job_kill.xml",
                  "4.root_fault.root_3_leaves_0.1_job_kill.xml",
                  "4.root_fault.root_4_leaves_0.1_job_kill.xml",
                  "4.root_fault.root_1_leaf_0.2_job_kill.xml",
                  "4.root_fault.root_2_leaves_0.2_job_kill.xml",
                  "4.root_fault.root_3_leaves_0.2_job_kill.xml",
                  "4.root_fault.root_4_leaves_0.2_job_kill.xml",
                  "4.root_fault.root_1_leaf_0.4_job_kill.xml",
                  "4.root_fault.root_2_leaves_0.4_job_kill.xml",
                  "4.root_fault.root_3_leaves_0.4_job_kill.xml",
                  "4.root_fault.root_4_leaves_0.4_job_kill.xml",
                  "4.root_fault.root_1_leaf_0.8_job_kill.xml",
                  "4.root_fault.root_2_leaves_0.8_job_kill.xml",
                  "4.root_fault.root_3_leaves_0.8_job_kill.xml",
                  "4.root_fault.root_4_leaves_0.8_job_kill.xml",
                  "4.root_fault.root_1_leaf_1_job_kill.xml",
                  "4.root_fault.root_2_leaves_1_job_kill.xml",
                  "4.root_fault.root_3_leaves_1_job_kill.xml",
                  "4.root_fault.root_4_leaves_1_job_kill.xml",
                  "4.root_fault.all_0.1_job_kill.xml",
                  "4.root_fault.all_0.2_job_kill.xml",
                  "4.root_fault.all_0.4_job_kill.xml",
                  "4.root_fault.all_0.8_job_kill.xml",
                  "4.root_fault.all_1_job_kill.xml",
                  "4.root_fault.4_leaves_0.1_0.3_0.5_0.7_job_kill.xml" );
# the only difference between these is the effective job kill rates

my @test12KillRates = ( 0.1, 0.2, 0.4, 0.8, 1,
                        (1-0.9**2), (1-0.8**2), (1-0.6**2), (1-0.2**2), 1,
                        (1-0.9**3), (1-0.8**3), (1-0.6**3), (1-0.2**3), 1,
                        (1-0.9**4), (1-0.8**4), (1-0.6**4), (1-0.2**4), 1,
                        0.1, 0.2, 0.4, 0.8, 1,
                        (1-0.9**2), (1-0.9**3), (1-0.9**4), (1-0.9**5),
                        (1-0.8**2), (1-0.8**3), (1-0.8**4), (1-0.8**5),
                        (1-0.6**2), (1-0.6**3), (1-0.6**4), (1-0.6**5),
                        (1-0.2**2), (1-0.2**3), (1-0.2**4), (1-0.2**5),
                        1, 1, 1, 1,
                        (1-0.9**7), (1-0.8**7), (1-0.6**7), (1-0.2**7), 1,
                        (1-0.9*0.7*0.5*0.3) );

my $killRateAllowableError = 0.1;     # 10%
my $errorRateAllowableError = 0.075;     # 7.5%

foreach $count (0 .. $#test12XML) {

  $XMLiter = $test12XML[ $count ];
  $killRateIter = $test12KillRates[ $count ];

  system( "echo \"testing with $xmlDir/$XMLiter\n\" > $testLog" );
  system( "$SSTloc/sst $SSTargs$xmlDir/$XMLiter >> $testLog" );

  die "Test 12.$count failed, fault[s] were not generated.  See $testLog\n" unless -s $faultLog;
  die "Test 12.$count failed, error[s] were not generated.  See $testLog\n" unless -s $errorLog;

  open ERRORLOG, $errorLog or die "Could not open the error log: $errorLog";
  $errorCount = 0;

  while( <ERRORLOG> ){
    m/([^,]+),[^,]+,[^,]+/;
    if( $1 ne "time" ){
      ++ $errorCount;
    }
  }
  close ERRORLOG;

  open JOBLOG, $jobLog or die "Could not open the job log: $jobLog";
  $killedJobs = 0;
  $simEndTime = 0;
  while( <JOBLOG> ){
    m/([^,]+),([^,]+),([^,]+),([^,]+),[^,]+/;
    ++$killedJobs if $4 eq "1";       # this should only show up in the log if a job was killed
    $simEndTime = $3;
  }
  close JOBLOG;

  $errorRate = $simEndTime / $errorCount;
  $lowErrorBound = (1-$errorRateAllowableError) * 864.49426;
  $highErrorBound = (1+$errorRateAllowableError) * 864.49426;
    # that magic number was calculated by averaging 50k values from the error time generation function assuming a lambda of 100.

  die "Test 12.$count failed, error rate was too low (observed < bound): $errorRate < $lowErrorBound.  See $testLog\n" if $errorRate < $lowErrorBound; 
  die "Test 12.$count failed, error rate was too high (observed > bound): $errorRate > $lowErrorBound.  See $testLog\n" if $errorRate > $highErrorBound; 

  $killRate = $killedJobs / $errorCount;
  $lowKillBound = (1-$killRateAllowableError) * $killRateIter;
  $highKillBound = (1+$killRateAllowableError) * $killRateIter;
  
  die "Test 12.$count failed, kill rate too low (observed < bound): $killRate < $lowKillBound.  See $testLog\n" if $killRate < $lowKillBound;
  die "Test 12.$count failed, kill rate too high (observed > bound): $killRate > $highKillBound.  See $testLog\n" if $killRate > $highKillBound;

  print "test 12.$count passed\n";

  system( $cleancmd );
}


# test the error correction


  # test to ensure that all three main parts of the graph correct errors correctly.
@correctedXML = ( "4.leavesfaultcorrect.xml", "4.middlefaultcorrect.xml", "4.rootfaultcorrect.xml" );

foreach $count (0 .. $#correctedXML){

  $XMLfile = $correctedXML[ $count ];
  $testNum = "13.$count";

  system( "echo \"testing with $xmlDir/$XMLfile\n\" > $testLog" );
  system( "$SSTloc/sst $SSTargs$xmlDir/$XMLfile >> $testLog" );

  open JOBLOG, $jobLog or die "Could not open the job log: $jobLog";
  while( <JOBLOG> ){
    m/([^,]+),([^,]+),([^,]+),([^,]+),[^,]+/;
    die "Test $testNum failed, job $1 died when the fault was corrected.  See $testLog\n" if $4 eq '1';
  }
  close JOBLOG;

  print "test $testNum passed\n";

  system( $cleancmd );
}

  # test when each leaf can correct errors with p=0.75
system( "echo \"testing with $xmlDir/4.sometimesfaultcorrect.xml\n\" > $testLog" );
system( "$SSTloc/sst $SSTargs$xmlDir/4.sometimesfaultcorrect.xml >> $testLog" );
$testNum = "13." . ($#correctedXML + 1);

$startedJobs = 0;
$passedJobs = 0;
$failedJobs = 0;

open ERRORLOG, $errorLog or die "Could not open the error log: $errorLog";
$errorCount = 0;

while( <ERRORLOG> ){
  m/([^,]+),[^,]+,[^,]+/;
  if( $1 ne "time" ){
    ++ $errorCount;
  }
}
close ERRORLOG;

open JOBLOG, $jobLog or die "Could not open the job log: $jobLog";
$killedJobs = 0;
$simEndTime = 0;
while( <JOBLOG> ){
  m/([^,]+),([^,]+),([^,]+),([^,]+),[^,]+/;
  ++$killedJobs if $4 eq "1";       # this should only show up in the log if a job was killed
  $simEndTime = $3;
}
close JOBLOG;

$killRate = $killedJobs / $errorCount;
  # all four leaf nodes have a .75 probability of correcting the fault.
  # expected kill rate should be about 1-(0.75**4)

$expectedKillRate = 1 - (0.75 ** 4);

die "Test $testNum failed, error correction rate observed to be too low (observed job kills > expected job kills): $killRate > " . ($expectedKillRate * 1.05) . ".  See $testLog\n" if ($killRate > ($expectedKillRate * 1.05));
die "Test $testNum failed, error correction rate observed to be too high (observed job kills < expected job kills): $killRate < " . ($expectedKillRate * 0.95) . ".  See $testLog\n" if ($killRate < ($expectedKillRate * 0.95));

print "test $testNum passed\n";
system( $cleancmd );

  # test when no errors are corrected.
system( "echo \"testing with $xmlDir/4.nofaultcorrect.xml\n\" > $testLog" );
system( "$SSTloc/sst $SSTargs$xmlDir/4.nofaultcorrect.xml >> $testLog" );
$testNum = "13." . ($#correctedXML + 2);

$startedJobs = 0;
$passedJobs = 0;
$failedJobs = 0;

open ERRORLOG, $errorLog or die "Could not open the error log: $errorLog";
$errorCount = 0;

$lastTime = 0;

$errorsOneSecondLater = 0;
  # errors can occur one second after a previous error.  Currently, all messages have the same priority,
  # so a job may start either before or after a fault, even if they are on the same timestep.  Keep track
  # of the number of times two errors occur within one timestep of each other, and use this to bound the
  # number of jobs killed.

while( <ERRORLOG> ){
  m/([^,]+),[^,]+,[^,]+/;
  if( $1 ne "time" ){
    ++ $errorCount;
    if( $1 - $lastTime <= 1 ){
      ++ $errorsOneSecondLater;
    }

    $lastTime = $1;
  }
}
close ERRORLOG;

open JOBLOG, $jobLog or die "Could not open the job log: $jobLog";
$killedJobs = 0;
$simEndTime = 0;
while( <JOBLOG> ){
  m/([^,]+),([^,]+),([^,]+),([^,]+),[^,]+/;
  ++$killedJobs if $4 eq "1";       # this should only show up in the log if a job was killed
  $simEndTime = $3;
}
close JOBLOG;

die "Test $testNum failed, errors were corrected when they were not supposed to be.  See $testLog\n" if ($killedJobs <= ($errorCount - $errorsOneSecondLater));

print "test $testNum passed\n";
system( $cleancmd );


# test that all jobs were actually run - trust nothing
# this should test all the things we might take for granted,
# like "jobs start before they finish" and "all the jobs will be run".
# Just add any XML graphs that have caused problems.


my @bigTestXML = ("4.4999_jobs.xml",
                  "4.5000_jobs.xml",
                  "4.5001_jobs.xml",
                  "4.5002_jobs.xml",
                  "4.5003_jobs.xml",
                  "4.5004_jobs.xml",
                  "4.5005_jobs.xml",
                  "4.25000_jobs.xml",
                  "8.300_jobs.xml",
                  "a.xml",
                  "b.xml");

my @bigTestJobs = ("4999_4_nodes.csv",
                   "5000_4_nodes.csv",
                   "5001_4_nodes.csv",
                   "5002_4_nodes.csv",
                   "5003_4_nodes.csv",
                   "5004_4_nodes.csv",
                   "5005_4_nodes.csv",
                   "25000_4_nodes.csv",
                   "300_8_nodes.csv",
                   "a_b.csv",
                   "a_b.csv");

foreach $testIndex (0 .. $#bigTestXML){
  $testNum = "14.$testIndex";

  $xmlIter = $bigTestXML[ $testIndex ];
  $jobsIter = $bigTestJobs[ $testIndex ];

  system( "echo \"testing with $xmlDir/$xmlIter\n\" > $testLog" );
  system( "$SSTloc/sst $SSTargs$xmlDir/$xmlIter >> $testLog" );

  %jobs = ();
  
  $startedJobs = 0;
  $passedJobs = 0;
  $failedJobs = 0;

  open JOBLOG, $jobLog or die "Could not open the job log: $jobLog";
  while( <JOBLOG> ){
    m/([^,]+),([^,]+),([^,]+),([^,]+),[^,]+/;

    ++ $startedJobs if $4 eq '-1';
    ++ $passedJobs if $4 eq '0';
    ++ $failedJobs if $4 eq '1';

    if( !$jobs{ $1 } ){
      $jobs{ $1 } = 1;
    }
    else{
      die "Test $testNum failed, Job $1 finished before it started.  See $testLog\n" if $4 =~ /-1/;
      $jobs{ $1 } ++;
    }
  }

  open JOBLIST, "traces/$jobsIter" or die "could not open the job list: traces/$jobsIter";

  while( <JOBLIST> ){
    m/([^,]+),[^,]+,[^,]+/;
    die "Test $testNum failed, job $1 doesn't appear to have both a start and a finish log entry.  See $testLog\n" if $_ =~ /^\d/ and $jobs{ $1 } != 2;
  }

  die "Test $testNum failed, not all jobs finished running.  See $testLog\n" if $startedJobs > ($passedJobs + $failedJobs);
  die "Test $testNum failed, more jobs finished than started.  See $testLog\n" if $startedJobs < ($passedJobs + $failedJobs);

  print "test $testNum passed\n";

  system( $cleancmd );
}

