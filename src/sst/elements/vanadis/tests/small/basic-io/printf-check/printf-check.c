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

int main(int argc, char* argv[]) {
	int value = 1234567;

	if(argc > 1) {
		value = atoi(argv[1]);
	}

	double value_dbl = (double) value;
	float  value_flt = (float) value;

	long long int value_lli = (long long int) value;
   unsigned int value_ui = (unsigned int) value;
	unsigned long long int value_uli = (unsigned long long int) value;

	printf("value (int) is %d\n", value);
	printf("value (dbl) is %f\n", value_dbl);
	printf("value (flt) is %f\n", value_flt);
	printf("value (lli) is %lld\n", value_lli);
	printf("value (usi) is %u\n", value_ui);
   printf("value (uli) is %llu\n", value_uli);
	printf("value is (int) is %d in the middle of a string\n", value);

   return 0;
}
