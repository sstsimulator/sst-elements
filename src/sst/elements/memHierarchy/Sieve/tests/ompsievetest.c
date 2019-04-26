// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <stdio.h>
#include <stdlib.h>
#include <vector>

/*
 *  Test for sieve
 *  Contains:
 *      * direct calls to malloc
 *      * indirect calls (via std library containers)
 *      * calls from each thread
 *      * accesses to mallocs by different threads
 *
 */
int main(int argc, char* argv[]) {

    const int n = 20;
    int** the_array = (int**) malloc(sizeof(int*) * n);

    int i = 0;
    #pragma omp parallel for
    for(i = 0; i < n; ++i) {
    	the_array[i] = (int*) malloc(sizeof(int) * n);
        int j = 0;
        for (j = 0; j < n; ++j) {
            the_array[i][j] = 0;    // initialize
        }
    }

    #pragma omp parallel for
    for(i = 0; i < n; ++i) {
    	int j = 0;
    
        for(j = 0; j < n; ++j) {
	    if (j < i) {
		the_array[i][j] = 1;
	    } else {
	        the_array[i][j] = 0;
	    }
	}
    }
    
    // Now have a triangle matrix, no do something with std lib
    std::vector<int> rowSums;
    for (int i = 0; i < n; i++) {
        rowSums.push_back(0);
        for (int j = 0; j < n; j++) {
            rowSums[i] += the_array[i][j];
        }
    }

    printf("The vector is:\n");
    for (std::vector<int>::iterator it = rowSums.begin(); it != rowSums.end(); it++) {
	    printf("%d\n", *it);
    }

}
