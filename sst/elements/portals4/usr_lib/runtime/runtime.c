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
#include <stdint.h>
#include <errno.h>


#ifdef USE_M5
#define SYS_mmap_dev 274
static uint64_t volatile * ptr = NULL;
static long _m5_syscall( long number, long arg0, long arg1, long arg2 );
#else
static int* _barrierPtr;
#endif

static __attribute__ ((constructor)) void init(void)
{
#ifdef USE_M5
    char* tmp = getenv("SYSCALL_ADDR");
    unsigned long addr = 0;
    if ( tmp )
        addr = strtoul(tmp,NULL,16);


    ptr = (int64_t*)syscall( SYS_mmap_dev, addr, 0x2000 );
    //printf("%s():%d paddr=%#lx vaddr=%p\n",__func__,__LINE__,addr,ptr);
    assert( ptr );
#else

    int fd;
    if ((fd=open("/dev/p4", O_RDWR|O_SYNC))<0)
    {   
        perror("open");
        exit(-1);
    }
        
    void* devPtr =  mmap(0, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    assert( devPtr != 0 );
    _barrierPtr = (devPtr + 4096) - sizeof(*_barrierPtr); 
#endif
}

int cnos_get_rank( void )
{
    char* tmp = getenv("RT_RANK");
    int rank = -1;
    if ( tmp ) {
         rank = atoi(tmp);
    }
//    printf("%s():%d rank=%d\n",__func__,__LINE__,rank);

    return rank;
}

int cnos_get_size( void )
{
    char* tmp = getenv("RT_SIZE");
    int size = 0;
    if ( tmp ) {
         size = atoi(tmp);
    }
//    printf("%s():%d size=%d\n",__func__,__LINE__,size);

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
//    printf("%s():%d size=%d\n",__func__,__LINE__,size);
    return size;
}

int cnos_barrier( void )
{
//printf("%s():%d\n",__func__,__LINE__);
#ifdef USE_M5
    return _m5_syscall(500,0,0,0);
#else
    *_barrierPtr = 1;
    while( *_barrierPtr );
#endif
}


#ifdef USE_M5
static long _m5_syscall( long number, long arg0, long arg1, long arg2 )
{
    ptr[0] = arg0;
    ptr[1] = arg1;
    ptr[2] = arg2;

    //asm volatile("wmb");
    ptr[0xf] = number + 1;
    //asm volatile("wmb");

    while ( (signed) ptr[0xf] == number + 1 );

    long retval = ptr[0];

    if ( retval < 0 ) {
        errno = -retval;
        retval = -1;
    }
    return retval;
}
#endif

