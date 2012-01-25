#include <unistd.h>
#include <stdlib.h>
#include <runtime.h>
#include <assert.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>

static int* _barrierPtr;

static __attribute__ ((constructor)) void init(void)
{
    int fd;
    if ((fd=open("node", O_RDWR|O_SYNC))<0)
    {   
        perror("open");
        exit(-1);
    }
        
    void* devPtr =  mmap(0, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    assert( devPtr != 0 );
    _barrierPtr = (devPtr + 4096) - sizeof(*_barrierPtr); 
}

int cnos_get_rank( void )
{
    char* tmp = getenv("RT_RANK");
    int rank = -1;
    if ( tmp ) {
         rank = atoi(tmp);
    }

    return rank;
}

int cnos_get_size( void )
{
    char* tmp = getenv("RT_SIZE");
    int size = 0;
    if ( tmp ) {
         size = atoi(tmp);
    }

    return size;
}

int cnos_get_nidpid_map( cnos_nidpid_map_t** map )
{
    int size = cnos_get_size();
    if ( size > 0) {
        *map = (cnos_nidpid_map_t*) malloc( sizeof( cnos_nidpid_map_t ) * size );
        int i;
        for ( i = 0; i < size; i++ ) {
            (*map)[i].nid = i;  
            (*map)[i].pid = getpid();
        }
    }
    return size;
}

int cnos_barrier( void )
{
    *_barrierPtr = 1;
    while( *_barrierPtr );
}
