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

#include <stdio.h>
#include <rdma.h>
#include <unistd.h>

#include <stdlib.h>

int main( int argc, char* argv[] ) {

	rdma_init();

#if 0
	char recvBuf[100];
	char sendBuf[100];
#endif

	setvbuf(stdout, NULL, _IONBF, 0);

	int myNode = rdma_getMyNode();
	int numNodes = rdma_getNumNodes();

	printf("%s() myNode=%d numNodes=%d\n",__func__,myNode,numNodes);

	int cq = rdma_create_cq( );
	printf("cq=%d\n",cq);

	size_t length = 1024;
	volatile int* addr = malloc( length ); 
	
	MemRgnKey key = 0xf00d;
	rdma_memory_reg( key, (void*) addr, length );

		Pid pid = 0;
	if ( myNode == 0 ) {
		addr[0] = 0;
		printf("%s() 0, read local mem, addr=%p\n",__func__,addr);
		while ( addr[0] == 0 );
		printf("%s() 0, read local mem, value %x\n",__func__,addr[0]);

		Node node = 1; 
		volatile int tmp = 0;
		
		printf("%s() 0, addr of tmp  %p\n",__func__,&tmp);
		rdma_memory_read( key, node, pid, 0, (void*) &tmp, sizeof(tmp), cq, 0 ); 	
	
		while ( tmp == 0 );
		printf("%s() 0, read far mem value %x\n",__func__,tmp);
        RdmaCompletion comp;
        rdma_read_comp( cq, &comp, 1 );
		printf("%s() 0, got read completion \n",__func__);

	} else {
		addr[0] = 10;
		int tmp=0x1234abcd;
			
		Node node = 0; 
		rdma_memory_write( key, node, pid, 0, &tmp, sizeof(tmp), cq, 0 ); 	
		while ( addr[0] == 0 );
		printf("%s() 1, read local mem, value %x\n",__func__,addr[0]);
	}


	rdma_fini();
	printf("%s() %d returning\n",__func__,myNode);
}
