// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <malloc.h>
#include <stdio.h>

int main( int argc, char ** argv ){
  int opt;

  unsigned int numJobs = 10;
  unsigned int maxNumNodes = 4;
  unsigned int maxJobLength = 1000;

  unsigned int nodeNumSeed;
  unsigned int jobLengthSeed;

  while( (opt = getopt( argc, argv, "n:j:l:a:b:" )) != -1 ){
    switch( opt ){
      case 'n':
        maxNumNodes = atoi( optarg );
        break;
      case 'j':
        numJobs = atoi( optarg );
        break;
      case 'l':
        maxJobLength = atoi( optarg );
        break;
      case 'a':
        nodeNumSeed = atoi( optarg );
        break;
      case 'b':
        jobLengthSeed = atoi( optarg );
        break;
    }
  }

  unsigned long long int counter = 0;

  for( ; counter < numJobs; counter ++ ){
    printf( "%llu, %i, %i\n", counter, (rand_r( &jobLengthSeed ) % maxJobLength) + 1, (rand_r( &nodeNumSeed ) % maxNumNodes) + 1 );
  }

  printf( "YYKILL" );

  return 0;
}
