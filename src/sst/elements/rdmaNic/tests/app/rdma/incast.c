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

//	uint32_t* buf = malloc( BUF_SIZE * sizeof(uint32_t) );
//	uint32_t buf[BUF_SIZE];

//	bzero( (void*) buf, BUF_SIZE * sizeof(uint32_t) );
	setvbuf(stdout, NULL, _IONBF, 0);

	int myNode = rdma_getMyNode();
	int numNodes = rdma_getNumNodes();

	printf("%s() myNode=%d numNodes=%d\n",__func__,myNode,numNodes);
		
	int msgCq = rdma_create_cq( );

	int cq = rdma_create_cq( );

	int rq = rdma_create_rq( 0xbeef, msgCq );

    printf("%s() call barrier\n",__func__);
    rdma_barrier();
    printf("%s() barrier returned\n",__func__);


	for ( int i =0; i < numNodes -1 ; i++ ) {
		void* buf = g_buf[i];
		printf("receive buffer=%p\n",buf);
		rdma_recv_post( (void*)buf, BUF_SIZE * sizeof(uint32_t), rq, (Context) buf );
	}
	
	if ( 0 == myNode ) {
		for ( int i = 0; i < numNodes -1; i++ ) {
			RdmaCompletion comp;
			rdma_read_comp( msgCq, &comp, 1 );
			printf("got a message in buffer %#x\n",comp.context);
#if VALIDATE
			uint32_t* buf = (uint32_t*) comp.context;
			for ( int i = 0; i < BUF_SIZE; i++ ) {
				if ( buf[i] !=  i ) {
					printf("Error: %p index=%d != %d\n",&buf[i],i,buf[i]);
				}
			}
#endif
		}
	} else {
		uint32_t* buf = g_buf[0];
		printf("send buffer %p\n",buf);
#if VALIDATE
		for ( int i = 0; i < BUF_SIZE; i++ ) {
			buf[i] = i;
		}
#endif
		rdma_send_post( (void*)buf, BUF_SIZE * sizeof(uint32_t), 0, 0, 0xbeef, cq, 0xf00dbeef ); 
		RdmaCompletion comp;
		rdma_read_comp( cq, &comp, 1 );
		printf("message sent\n");
	}

	rdma_fini();
	printf("%s() returning\n",__func__);
}
