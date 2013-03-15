$numJobs = 25000;
$numNodes = 4;
$jobLen = 1000;

foreach $jobNum (1 .. ($numJobs + 1) ){
  print "$jobNum, $jobLen, $numNodes\n";
}

print "YYKILL";

