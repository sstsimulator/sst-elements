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

#if 0
#define SYS_virt2phys 442
#define SYS_mmap_dev  443   
#else
#define SYS_virt2phys 273 
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

int __libc_open( const char *file, int oflag, ... )
{
    va_list ap;
    mode_t mode;

    va_start( ap, oflag );
    if ( oflag & O_CREAT ) {
	mode = va_arg( ap, mode_t );
    } 
    va_end( ap );
        
    return my_syscall( SYS_open, (long) syscall(SYS_virt2phys,file), 
			(long) oflag, (long) mode );
}

weak_alias( __libc_open, open )
weak_alias( __libc_open, open64 )
weak_alias( __libc_open, __open )
weak_alias( __libc_open, __open_nocancel )

int __libc_close( int fd )
{
    return my_syscall( SYS_close, (long) fd, 0, 0 );
}

weak_alias( __libc_close, close )
weak_alias( __libc_close, __close )
weak_alias( __libc_close, __close_nocancel )

ssize_t __libc_write( int fd, const void *buf, size_t n )
{
    return my_syscall( SYS_write, (long) fd, 
			(long) syscall( SYS_virt2phys, buf ), (long) n );
}

weak_alias( __libc_write, write )
weak_alias( __libc_write, __write )
weak_alias( __libc_write, __write_nocancel )

ssize_t __libc_read( int fd, void *buf, size_t n )
{
    return my_syscall( SYS_read, (long) fd,
			(long) syscall( SYS_virt2phys, buf ), (long) n );
}

weak_alias( __libc_read, read )
weak_alias( __libc_read, __read )
weak_alias( __libc_read, __read_nocancel )

void exit( int status )
{
    my_syscall( SYS_exit, status, 0, 0 );
    while(1);
}
