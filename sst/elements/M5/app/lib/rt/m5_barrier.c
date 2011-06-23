#include <sys/syscall.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

#include <m5rt.h>

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

int cnos_barrier( void )
{
    return my_syscall(500,0,0,0);
}
