#!/usr/bin/perl -w

sub getLink($$) {
    my $a = shift;
    my $b = shift;

    if ($a < $b) {
	return "$a.$b";
    } else{
	return "$b.$a";
    }
}

sub getCNum($$) {
    my $inX = shift;
    my $inY = shift;
    
    return ($x * $inY) + $inX;
}

$x = $ARGV[0];
$y = $ARGV[1];
$set = $ARGV[2];

if ($set == 1) {
    $workPerCycle = 200;
    $commFreq = 100000;
    $commSize = 100;
    $commLat = "100 ns";
} else {
    $set = 0;
    $workPerCycle = 100;
    $commFreq = 100000;
    $commSize = 100;
    $commLat = "10 ns";
}

printf("making ${x}x${y}, setup $set\n", $x, $y);

$fileN = "${x}x${y}-$set";
open(OUT, ">$fileN.xml");

printf(OUT "<?xml version=\"1.0\"?>\n".
       "<config>\n".
       " stopAtCycle=25us\n".
       "</config>\n".
       "<sst>\n");

for ($yi = 0; $yi < $y; ++$yi) {
    for ($xi = 0; $xi < $x; ++$xi) {

	#compute link names
	$cn = getCNum($xi, $yi);
	printf("Component %d,%d=%d\n", $xi, $yi, $cn);
	# North
	$nx = $xi;
	$ny = $yi + 1;
	if ($ny >= $y) {$ny = 0;}
	$nn = getCNum($nx, $ny);
	$NLink = getLink($nn,$cn);
	printf(" N: %d %s\n", $nn, $NLink);

	# South
	$nx = $xi;
	$ny = $yi - 1;
	if ($ny < 0) {$ny = $y-1;}
	$nn = getCNum($nx, $ny);
	$SLink = getLink($nn,$cn);
	printf(" S: %d %s\n", $nn, $SLink);

	# East
	$nx = $xi + 1;
	if ($nx >= $x) {$nx = 0;}
	$ny = $yi;
	$nn = getCNum($nx, $ny);
	$ELink = getLink($nn,$cn);
	printf(" E: %d %s\n", $nn, $ELink);

	# West
	$nx = $xi - 1;
	if ($nx < 0) {$nx = $x-1;}
	$ny = $yi;
	$nn = getCNum($nx, $ny);
	$WLink = getLink($nn,$cn);
	printf(" W: %d %s\n", $nn, $WLink);
	

	print OUT<<EOT
  <component id="c$xi.$yi">
    <simpleComponent.simpleComponent>
      <params>
	<workPerCycle>$workPerCycle</workPerCycle>
	<commFreq>$commFreq</commFreq>
	<commSize>$commSize</commSize>
      </params>
      <links>
	<link id="${NLink}">
	  <params>
	    <name>Nlink</name>
	    <lat>$commLat</lat>
	  </params>
	</link>

	<link id="$SLink">
	  <params>
	    <name>Slink</name>
	    <lat>$commLat</lat>
	  </params>
	</link>

	<link id="$ELink">
	  <params>
	    <name>Elink</name>
	    <lat>$commLat</lat>
	  </params>
	</link>

	<link id="$WLink">
	  <params>
	    <name>Wlink</name>
	    <lat>$commLat</lat>
	  </params>
	</link>
      </links>
    </simpleComponent.simpleComponent>
  </component>

EOT
    }
}

printf(OUT "</sst>");

close(OUT);
