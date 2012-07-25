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
static int _readFd;
static int _writeFd;
#else
static int* _barrierPtr;
#endif

static __attribute__ ((constructor)) void init(void)
{
#ifdef USE_M5
    char buf[100];

    sprintf( buf, "/tmp/sst-barrier.%d", getppid() );

    _writeFd = open( buf, O_WRONLY );
    assert( _writeFd > -1 );

    sprintf( buf, "/tmp/sst-barrier-app-%s.%d", 
                    getenv("BARRIER_UNIQUE"), getppid() );

    _readFd = open( buf, O_RDONLY | O_NONBLOCK );
    assert( _readFd > -1 );
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
#ifdef USE_M5
            (*map)[i].pid = getpid();
#else
            (*map)[i].pid = 100;
#endif
        }
    }
//    printf("%s():%d size=%d\n",__func__,__LINE__,size);
    return size;
}

int cnos_barrier( void )
{
//printf("%s():%d\n",__func__,__LINE__);
#ifdef USE_M5
    int buf = getpid();
    int rc;
    rc = write( _writeFd, &buf, sizeof(buf) );
    assert( rc == sizeof(buf) );

    while ( ( rc = read( _readFd, &buf, sizeof(buf) ) ) == 0 || 
                                        ( rc == -1 && errno == EAGAIN ) );
    assert( rc == sizeof(buf) );
    return 0;
#else
    *_barrierPtr = 1;
    while( *_barrierPtr );
#endif
}
