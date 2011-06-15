#include <m5rt.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>

#  define weak_alias(name, aliasname) _weak_alias (name, aliasname)
#  define _weak_alias(name, aliasname) \
  extern __typeof (name) aliasname __attribute__ ((weak, alias (#name)));

#if 1 
#define SYS_mmap_dev  443   
#else
#define SYS_mmap_dev  274 
#endif

static uint64_t volatile * ptr;

static __attribute__ ((constructor)) void init(void)
{
    char* tmp = getenv("SYSCALL_ADDR");
    unsigned long addr = 0;
    if ( tmp )
        addr = strtoul(tmp,NULL,16);

    ptr  = (uint64_t*)syscall( SYS_mmap_dev, addr, 0x2000 );
}

static inline long my_syscall( long number, long arg0, long arg1, long arg2 )
{
    ptr[0] = arg0;
    ptr[1] = arg1;
    ptr[2] = arg2;

    //asm volatile("wmb");
    ptr[0xf] = number + 1;
    //asm volatile("wmb");

    while ( ptr[0xf] == number + 1 );

    long retval = ptr[0];

    if ( retval < 0 ) {
        errno = -retval;
        retval = -1;
    }
    return retval;
}


int m5_get_rank( void )
{
    char* tmp = getenv("RANK");
    int rank = -1;
    if ( tmp ) {
         rank = atoi(tmp);
    }

    return rank;
}

int m5_get_size( void )
{
    char* tmp = getenv("SIZE");
    int rank = 0;
    if ( tmp ) {
         rank = atoi(tmp);
    }

    return rank;
}

int m5_barrier( void )
{
    return my_syscall(500,0,0,0);
}

int m5_get_nidpid_map( m5_nidpid_map_t** map )
{
    int size = m5_get_size();
    if ( size > 0) {
        *map = malloc( sizeof( m5_nidpid_map_t ) * size );
        int i;
        for ( i = 0; i < size; i++ ) {
            (*map)[i].nid = i;  
            (*map)[i].pid = getpid();
        }
    }
    return size;
}
