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
		printf("%s() 0, read local mem, addr=%x\n",__func__,(int)addr);
		while ( addr[0] == 0 );
		printf("%s() 0, read local mem, value %x\n",__func__,addr[0]);

		Node node = 1; 
		volatile int tmp = 0;
		
		printf("%s() 0, addr of tmp  %p\n",__func__,&tmp);
		rdma_memory_read( key, node, pid, 0, (void*) &tmp, sizeof(tmp), cq, (int)NULL ); 	
	
		while ( tmp == 0 );
		printf("%s() 0, read far mem value %x\n",__func__,tmp);
        RdmaCompletion comp;
        rdma_read_comp( cq, &comp, 1 );
		printf("%s() 0, got read completion \n",__func__);

	} else {
		addr[0] = 10;
		int tmp=0x1234abcd;
			
		Node node = 0; 
		rdma_memory_write( key, node, pid, 0, &tmp, sizeof(tmp), cq, (int)NULL ); 	
		while ( addr[0] == 0 );
		printf("%s() 1, read local mem, value %x\n",__func__,addr[0]);
	}


	rdma_fini();
	printf("%s() %d returning\n",__func__,myNode);
}
