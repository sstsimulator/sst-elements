#!/usr/bin/perl

# creates a completely flat graph, fully connected to the scheduler.
# Assumes that yumyum functionality is desired.

use Getopt::Long;

my $numNodes = 3;
my $jobList = "joblist.csv";
my $jobLog = "job.log";
my $printHelp;

GetOptions( "nodes=n" => \$numNodes,
            "jobList=s" => \$jobList,
            "jobLog=s" => \$jobLog,
            "help" => \$printHelp );

if( $printHelp ){
  print "Flat graph generator for yumyum\n" .
        "--nodes=[number of nodes, count from zero] (default=3)\n" .
        "--jobList=[filename of the job list] (default=joblist.csv)\n" .
        "--jobLog=[filename of the job log] (default=job.log)\n";
  exit 0;
}

print "<?xml version=\"1.0\"?>\n" .
      "<sdl version=\"2.0\" />\n\n" .
      "<sst>\n";

for $counter (0 .. $numNodes){
  print "  <component name=\"1." . ($counter + 1) . "\" type=\"scheduler.nodeComponent\">\n" .
        "    <link name=\"schedLink$counter\" latency=\"0 ns\" port=\"Scheduler\" />\n" .
        "    <params>\n" .
        "      <id>1." . ($counter + 1) . "</id>\n" .
        "      <nodeNum>$counter</nodeNum>\n" .
        "    </params>\n" .
        "  </component>\n";
}

print "  <component name=\"scheduler\" type=\"scheduler.schedComponent\">\n";

for $counter (0 .. $numNodes){
  print "    <link name=\"schedLink$counter\" latency=\"0 ns\" port=\"nodeLink$counter\" />\n";
}

print "    <params>\n".
      "      <traceName>$jobList</traceName>\n" .
      "      <jobLogFileName>$jobLog</jobLogFileName>\n" .
      "      <useYumYumTraceFormat>true</useYumYumTraceFormat>\n" .
      "      <printYumYumJobLog>true</printYumYumJobLog>\n" .
      "      <printJobLog>true</printJobLog>\n" .
      "    </params>\n" .
      "  </component>\n" .
      "</sst>";

