
// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <stddef.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char* argv[]) {

    setvbuf(stdout, NULL, _IONBF ,0);

#ifdef _OPENMP
    int k = 0;
#pragma omp parallel
    {
#pragma omp atomic
        k++;
    }
    printf ("Number of Threads counted = %i\n",k);
#endif

    printf("exit\n");
}

