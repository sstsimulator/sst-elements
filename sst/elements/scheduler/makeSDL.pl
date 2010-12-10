#!/usr/bin/perl -w

$n = $ARGV[0];

{
    print <<EOT
<?xml version="2.0"?>

<config>
    run-mode=both
    partitioner=self
</config>

<sst>
  <component name="s" type="scheduler.schedComponent" rank=0>
EOT
}

for ($i = 0; $i < $n; ++$i) {
    printf("   <link name=\"l$i\" port=\"nodeLink$i\" latency=\"1 ns\"/>\n");
}
printf(" </component>\n\n");

for ($i = 0; $i < $n; ++$i) {
print <<EOT
 <component name="n$i" type="scheduler.nodeComponent" rank=0>
   <params>
     <nodeNum>$i</nodeNum>
   </params>
   <link name="l$i" port="Scheduler" latency="1 ns"/>
 </component>
EOT
}

printf("</sst>\n");
