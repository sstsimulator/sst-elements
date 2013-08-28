#!/usr/bin/perl -w

$sdlname = "TempSdl.sdl";
$sstdir = "../../../core/";
$testoutdir = "tests";
@simfiles = ("LLNLshort.sim");
@numprocs = (4096);
@schedulers = ("cons[fifo]", "prioritize[1,fifo]", "delayed[fifo]", "elc[1,fifo]");
#@comparators = ("fifo");
@machines = ("simple[4096]");
#@allocators = ("sortedfreelist[sort]", "sortedfreelist[nosort]","sortedfreelist","sortedfreelist[hilbert]","sortedfreelist[snake]","sortedfreelist[sort,hilbert]","sortedfreelist[sort,snake]","sortedfreelist[nosort,hilbert]","sortedfreelist[nosort,snake]");
@allocators = ("simple");
@fst = ("strict", "relaxed");

foreach $sim (@simfiles)
{
foreach $n (@numprocs)
{
  foreach $scheduler (@schedulers)
  {
      #foreach $comparator (@comparators)
      #{
      foreach $machine (@machines)
      {
        foreach $allocator (@allocators)
        {
            foreach $fst (@fst)
            {
                if(!(-d $testoutdir))
                {
                    system("mkdir", $testoutdir);
                }
                #$schedulername = $scheduler."[".$comparator."]";
                $schedulername = $scheduler;
                $tracename = $sim.$schedulername.$machine.$allocator.$fst;
                $tracelocation = $testoutdir."/".$tracename;
                system("cp", $sim, $tracelocation);
                system("rm", $sdlname);
open(MYOUTFILE,">>".$sdlname);
{
    print MYOUTFILE <<EOT
<?xml version="2.0"?>

<sdl version="2.0"/>

<config>
    run-mode=both
    partitioner=self
</config>

<sst>
  <component name="s" type="scheduler.schedComponent" rank=0>
    <params>
      <traceName>$tracelocation</traceName>
	<scheduler>$schedulername</scheduler>
	<machine>$machine</machine>
	<allocator>$allocator</allocator>
        <FST>$fst</FST>
    </params>
EOT
}

for ($i = 0; $i < $n; ++$i) {
    printf MYOUTFILE ("   <link name=\"l$i\" port=\"nodeLink$i\" latency=\"0 ns\"/>\n");
}
printf MYOUTFILE (" </component>\n\n");

for ($i = 0; $i < $n; ++$i) {
print MYOUTFILE <<EOT
 <component name="n$i" type="scheduler.nodeComponent" rank=0>
   <params>
     <nodeNum>$i</nodeNum>
   </params>
   <link name="l$i" port="Scheduler" latency="0 ns"/>
 </component>
EOT
}

printf MYOUTFILE ("</sst>\n");
close MYOUTFILE;
$filearg = "--sdl-file=".$sdlname;
@args = ($sstdir."./sst.x",$filearg, "--verbose");
system(@args);
system("mv",$tracename.".alloc", $tracelocation.".alloc"); 
system("mv",$tracename.".time", $tracelocation.".time"); 
}
}
}
#}
}
}
}
