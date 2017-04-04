#!/usr/bin/perl -w
# this is meant to be called with an input address_map file and an input generated address sequence file from genSequence.pl
# it outputs a map of what structures each address hits according to the address_map
my $mapFile = $ARGV[0];
my $seqFile = $ARGV[1];

my %mapKey = ('C' => 'chan', 'R' => 'rank', 'B' => 'bankGroup', 'b' => 'bank', 'r' => 'row', 'l' => 'col', 'h' => 'cacheLine');

open(MAP,$mapFile) || die "Couldn't open address map file $mapFile: $!";

print "Parsing mapFile $mapFile\n";

my %maxValue;
my %bitPositions;

while(my $line = <MAP>) {
    if($line !~ /^#/) { # no comment at beginning of line
	my $curPos = 0;
	my ($bitLength, $fieldName);
	$line =~ s/\s+//g; #remove spaces
	$line =~ s/#.*//; #remove comments at end of line

	while($line) { # there's something left to parse
	    if($line =~ /:(\d+)$/) {
		$bitLength = $1;
		$line =~ s/:(\d+)$//;
	    } else {
		$bitLength = 1;
	    }

	    $fieldName = $mapKey{chop($line)};

	    for(my $ii=0;$ii<$bitLength;$ii++) {
		push(@{$bitPositions{$fieldName}}, $curPos);
		$curPos++;
	    }
	} # while there's something left to parse
    } # if line is not a comment
} # while(line = MAP)

close(MAP);

foreach my $struct (keys(%bitPositions)) {
    my $bitCount = $#{$bitPositions{$struct}}+1;
    $maxValue{$struct} = 2**$bitCount;
    print "$bitCount\t$struct: ";
    foreach my $val (@{$bitPositions{$struct}}) {
	print "$val ";
    } print "\n";
}

#print "bank max is $maxValue{'bank'}\n";

my $seqCount = 0;
open(SEQ,$seqFile) || die "Couldn't open input sequence file $seqFile: $!";

my @fileStructOrder = ('chan', 'rank', 'bankGroup', 'bank', 'row', 'col', 'cacheLine');

print "\n\n\t\t\t\tChan\tRank\tGroup\tbank\trow\tcol\t\$line\n";

while($line = <SEQ>) {
    if($line !~ /^#/ && $line =~ /fetch/) {
	@grep = split(/\s+/,$line);
	$addr = hex($grep[0]);

	chomp($line);
	print $line."\t\t";
	foreach my $struct (@fileStructOrder) {
	    $cur = 0;
	    $cnt = 0;
	    foreach my $val (@{$bitPositions{$struct}}) {
		#print "\n$struct $val\n";
		#$tt = ((1 << $val) & $addr) >> ($val - $cnt);
		#print "tt $tt $addr\n";
		$tmp = ((1 << $val) & $addr) >> ($val - $cnt);
		$cnt++;
		$cur |= $tmp;
	    }
	    print $cur."\t";
	}
	print "\n";
    } # if line not comment and =~ /fetch/
} # while(line = MAP)

close(SEQ);
