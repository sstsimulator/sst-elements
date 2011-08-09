#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>

#  define weak_alias(name, aliasname) _weak_alias (name, aliasname)
#  define _weak_alias(name, aliasname) \
  extern __typeof (name) aliasname __attribute__ ((weak, alias (#name)));

#include "m5_syscall.h"

#if 0
int __libc_open( const char *file, int oflag, ... )
{
    va_list ap;
    mode_t mode;

    va_start( ap, oflag );
    if ( oflag & O_CREAT ) {
	mode = va_arg( ap, mode_t );
    } 
    va_end( ap );
        
    return _m5_syscall( SYS_open, (long) syscall(SYS_virt2phys,file), 
			(long) oflag, (long) mode );
}

#if 1 
weak_alias( __libc_open, open )
weak_alias( __libc_open, open64 )
weak_alias( __libc_open, __open )
weak_alias( __libc_open, __open_nocancel )
#endif

int __libc_close( int fd )
{
    return _m5_syscall( SYS_close, (long) fd, 0, 0 );
}

#if 1 
weak_alias( __libc_close, close )
weak_alias( __libc_close, __close )
weak_alias( __libc_close, __close_nocancel )
#endif

ssize_t __libc_write( int fd, const void *buf, size_t n )
{
    return _m5_syscall( SYS_write, (long) fd, 
			(long) syscall( SYS_virt2phys, buf ), (long) n );
}

#if 1 
weak_alias( __libc_write, write )
weak_alias( __libc_write, __write )
weak_alias( __libc_write, __write_nocancel )
#endif

ssize_t __libc_read( int fd, void *buf, size_t n )
{
    return _m5_syscall( SYS_read, (long) fd,
			(long) syscall( SYS_virt2phys, buf ), (long) n );
}

#if 1 
weak_alias( __libc_read, read )
weak_alias( __libc_read, __read )
weak_alias( __libc_read, __read_nocancel )
#endif
#endif

void exit( int status )
{
    _m5_syscall( SYS_exit, status, 0, 0 );
    while(1);
}
