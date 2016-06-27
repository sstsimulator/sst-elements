#!/usr/bin/perl -w

$n = $ARGV[0];

if(scalar @ARGV == 5) {
    $FST = 'none'; 
}
else {
    $FST = $ARGV[5];
}


if(scalar @ARGV <= 6) {
    $timeperdistance = '0'; 
}
else {
    $timeperdistance = $ARGV[6];
}

if(scalar @ARGV <= 7) {
    $runningTimeSeed = 'none'; 
}
else {
    $runningTimeSeed = $ARGV[7];
}

if(scalar @ARGV <= 8) {
    $dMatrixFile = 'none'; 
}
else {
    $dMatrixFile = $ARGV[8];
}

{
    print <<EOT
<?xml version="1.0"?>
<sdl version="2.0"/>

<config>
    run-mode=both
</config>

<sst>
  <component name="s" type="scheduler.schedComponent" rank=0>
    <params>
      <traceName>$ARGV[1]</traceName>
	<scheduler>$ARGV[2]</scheduler>
	<machine>$ARGV[3]</machine>
	<allocator>$ARGV[4]</allocator>
    <FST>$FST</FST>
    <timeperdistance>$timeperdistance</timeperdistance>
    <runningTimeSeed>$runningTimeSeed</runningTimeSeed>
    <dMatrixFile>$dMatrixFile</dMatrixFile>
    </params>
EOT
}

for ($i = 0; $i < $n; ++$i) {
    printf("   <link name=\"l$i\" port=\"nodeLink$i\" latency=\"0 ns\"/>\n");
}
printf(" </component>\n\n");

for ($i = 0; $i < $n; ++$i) {
print <<EOT
 <component name="n$i" type="scheduler.nodeComponent" rank=0>
   <params>
     <nodeNum>$i</nodeNum>
   </params>
   <link name="l$i" port="Scheduler" latency="0 ns"/>
 </component>
EOT
}

printf("</sst>\n");
