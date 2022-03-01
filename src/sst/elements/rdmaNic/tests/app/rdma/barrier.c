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
