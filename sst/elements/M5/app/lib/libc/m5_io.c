
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>

#  define weak_alias(name, aliasname) _weak_alias (name, aliasname)
#  define _weak_alias(name, aliasname) \
  extern __typeof (name) aliasname __attribute__ ((weak, alias (#name)));

#include "m5_syscall.h"

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

weak_alias( __libc_open, open )
weak_alias( __libc_open, __open )
weak_alias( __libc_open, open64 )
weak_alias( __libc_open, __open64 )
weak_alias( __libc_open, __open_nocancel )

int __libc_close( int fd )
{
    return _m5_syscall( SYS_close, (long) fd, 0, 0 );
}

weak_alias( __libc_close, close )
weak_alias( __libc_close, __close )
weak_alias( __libc_close, __close_nocancel )

ssize_t __libc_write( int fd, const void *buf, size_t n )
{
    return _m5_syscall( SYS_write, (long) fd, 
			(long) syscall( SYS_virt2phys, buf ), (long) n );
}

weak_alias( __libc_write, write )
weak_alias( __libc_write, __write )
weak_alias( __libc_write, __write_nocancel )

ssize_t __libc_read( int fd, void *buf, size_t n )
{
    return _m5_syscall( SYS_read, (long) fd,
			(long) syscall( SYS_virt2phys, buf ), (long) n );
}

weak_alias( __libc_read, read )
weak_alias( __libc_read, __read )
weak_alias( __libc_read, __read_nocancel )
