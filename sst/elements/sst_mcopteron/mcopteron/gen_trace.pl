#!/usr/bin/perl

$code = 'MOVAPD 128 0 1 reg, reg
2
CVTSI2SD 64 0 1 reg, reg
5
ADD 32 0 1 reg, imm
6
CMP 32 0 1 imm
1
ADDSD 64 0 2 reg, reg
4
3
JCC 8 0 1 ptr
2
'; 

for($i=0; $i<1000000; $i++) { 
    print $code;
}



