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
#include <rdma.h>
#include <unistd.h>
#include <strings.h>

//#define BUF_SIZE 250000
#define BUF_SIZE 10

uint32_t g_buf[3][BUF_SIZE];

int main( int argc, char* argv[] ) {

	rdma_init();

	int myNode = rdma_getMyNode();
	int numNodes = rdma_getNumNodes();

	printf("%s() myNode=%d numNodes=%d\n",__func__,myNode,numNodes);
		
    printf("%s() call barrier\n",__func__);
    rdma_barrier();
    printf("%s() barrier returned\n",__func__);

	rdma_fini();
	printf("%s() returning\n",__func__);
}
