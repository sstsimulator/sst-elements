# args are:
# - max nodes a job can use
# - max seconds a job can take
# - number of jobs to create
# - seed for job length PRNG
# - seed for num nodes PRNG

use Math::Random::OO::UniformInt;

$numJobs = $ARGV[ 2 ];
$maxNumNodes = $ARGV[ 0 ];
$maxJobLen = $ARGV[ 1 ];

$jobLenPRNG = Math::Random::OO::UniformInt->new( 1, $maxJobLen );
$numNodesPRNG = Math::Random::OO::UniformInt->new( 1, $maxNumNodes );

$jobLenPRNG->seed( ($ARGV[ 3 ]) );
$numNodesPRNG->seed( ($ARGV[ 4 ]) );

foreach $jobNum (1 .. ($numJobs) ){
  print( "$jobNum, " . $jobLenPRNG->next() . ", " . $numNodesPRNG->next() . "\n" );
}

print "YYKILL";

