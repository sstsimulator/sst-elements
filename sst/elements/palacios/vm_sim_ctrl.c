
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>


int main( int argc, char* argv[] )
{
    int fd;
    int cmd;

    if ( argc != 2 ) {
        fprintf(stderr,"usage: %s [start|stop|term]\n",argv[0]);
        exit(-1);
    }

    if ( 0 == strncmp( argv[1], "start", 5 ) ) {
        cmd = 1;
    } else if ( 0 == strncmp( argv[1], "stop", 4 ) ) {
        cmd = 2;
    } else if ( 0 == strncmp( argv[1], "term", 4 ) ) {
        cmd = 3;
    } else { 
        fprintf(stderr,"usage: %s [start|stop|term]\n",argv[0]);
        exit(-1);
    }

    if ((fd=open("/dev/p4", O_RDWR|O_SYNC))<0)
    {
        perror("open");
        exit(-1);
    }


    void* devPtr =  mmap(0, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    assert( devPtr != 0 );

    // sizeof(*cmdPtr) * 2  because barrier uses the top int
    unsigned int* cmdPtr = (devPtr + 4096) - ( sizeof(*cmdPtr) * 2 );

    *cmdPtr = cmd;

    while ( *cmdPtr ) {
        sched_yield();
    }

    int ret = munmap( devPtr, 4096 );
    assert( ret == 0 );
    close( fd );
    return 0; 
}
