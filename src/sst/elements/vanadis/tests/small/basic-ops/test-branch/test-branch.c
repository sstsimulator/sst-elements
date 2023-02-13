// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main( int argc, char* argv[] ) {

	int max_value = 100000;

	int value_bins[3];
    bzero( value_bins, sizeof(int) * 3 );

	for( int i = 0; i < max_value; ++i ) {
		if( 0 == (i%3) ) {
			value_bins[0]++;
		} else if( 1 == (i%3) ) {
			value_bins[1]++;
		} else {
			// don't do anything
		}
	}

	printf("value-bins: 0 = %d, 1 = %d\n", value_bins[0], value_bins[1]);

	return 0;
}
