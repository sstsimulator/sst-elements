#!/usr/bin/perl -w

$app = $ARGV[0];  
$cubes = $ARGV[1];  
$vaults = $ARGV[2]; 
$threads = $ARGV[3]; 
$bwlimit = $ARGV[4]; 

$fileN = "app${app}-${cubes}x${vaults}-${threads}t-${bwlimit}bw";
open(OUT, ">$fileN.xml");

#header & cpu
print OUT<<EOT;
<?xml version="1.0"?>
<sdl version="2.0"/>
<config>
 stopAtCycle=50us
 partitioner=self
</config>
<sst>
  <component name="cpu" type=VaultSimC.cpu rank=0>
    <params>
      <clock>500Mhz</clock>
      <threads>$threads</threads>
      <app>$app</app>
      <bwlimit>$bwlimit</bwlimit>
    </params>
    <link name="chain_c_0" port="toMem" latency="5 ns" />
  </component> 
 
EOT

# lls
for ($i = 0; $i < $cubes; ++$i) {
    if ($i == $cubes-1) {
	$terminal = 1;
    } else {
	$terminal = 0;
    }

    $ll_mask = $cubes-1;

    print OUT<<EOT;
  <component name="ll$i" type=VaultSimC.logicLayer rank=0>
    <params>
      <clock>500Mhz</clock>
      <vaults>$vaults</vaults>
      <llID>$i</llID>
      <bwlimit>$bwlimit</bwlimit>
      <LL_MASK>${ll_mask}</LL_MASK>
      <terminal>$terminal</terminal>
    </params>
EOT

#ll vaults links
    for ($v = 0 ; $v < $vaults; ++$v) {
	printf(OUT "      <link name=\"ll2V_${i}_${v}\" port=\"bus_${v}\" latency=\"1 ns\" />\n");
    }

#ll connections
    if ($i == 0) { 
	# first one
	printf(OUT "    <link name=\"chain_c_0\" port=\"toCPU\" latency=\"5 ns\" />\n");
    } else {
	$m1 = $i - 1;
	printf(OUT "    <link name=\"chain_${m1}_${i}\" port=\"toCPU\" latency=\"5 ns\" />\n");
    } 

    if (! $terminal) {
	$m1 = $i + 1;
	printf(OUT "    <link name=\"chain_${i}_${m1}\" port=\"toMem\" latency=\"5 ns\" />\n");
    }

    printf(OUT "  </component>\n\n");

#Vaults
    $nv2 = log($vaults)/log(2);
    for ($v = 0 ; $v < $vaults; ++$v) {
	print OUT<<EOT;
  <component name="c$i.$v" type=VaultSimC.VaultSimC rank=0>
    <params>
      <clock>750Mhz</clock>
      <VaultID>$v</VaultID>
      <numVaults2>$nv2</numVaults2>
    </params>
    <link name="ll2V_${i}_${v}" port="bus" latency="1 ns" />
  </component>

EOT
    }
}

printf(OUT "</sst>\n\n");
close(OUT);
