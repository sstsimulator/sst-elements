#include <stdio.h>
#include <stdlib.h>
#include <rdma.h>
#include <unistd.h>
#include <strings.h>

#define BUF_SIZE 100

#define VALIDATE 1 

int main( int argc, char* argv[] ) {

	rdma_init();

	uint32_t* buf = malloc( BUF_SIZE * sizeof(uint32_t) );

	int myNode = rdma_getMyNode();
	int numNodes = rdma_getNumNodes();

	printf("%s() myNode=%d numNodes=%d\n",__func__,myNode,numNodes);
	printf("buffer=%p\n",buf);
		
	int cq = rdma_create_cq( );
	
	int rq = rdma_create_rq( 0xbeef, cq );
	
	if ( myNode == 0 ) {
#if VALIDATE 
		for ( int i = 0; i < BUF_SIZE; i++ ) {
			buf[i] = i;
		}
#endif
		rdma_send_post( (void*)buf, BUF_SIZE * sizeof(uint32_t), 1, 0, 0xbeef, cq, 0xf00dbeef ); 

		RdmaCompletion comp;
		rdma_read_comp( cq, &comp, 1 );
		
	} else {

		rdma_recv_post( (void*)buf, BUF_SIZE * sizeof(uint32_t), rq, 0x12345678 );
		RdmaCompletion comp;
		rdma_read_comp( cq, &comp, 1 );
#if VALIDATE 
		printf("%s() validate\n",__func__);
		for ( int i = 0; i < BUF_SIZE; i++ ) {
			if ( buf[i] !=  i ) {
				printf("Error: %p index=%d != %d\n",&buf[i],i,buf[i]);
			}
		}
		printf("%s() validate complete\n",__func__);
#endif
	}

	free( buf );
	rdma_fini();
	printf("%s() returning\n",__func__);
}
