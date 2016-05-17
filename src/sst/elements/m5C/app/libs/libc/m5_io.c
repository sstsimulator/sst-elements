// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#define _LARGEFILE64_SOURCE
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <assert.h>

#include <termios.h>
#include <asm/ioctls.h>

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
        
    int fd = _m5_syscall( SYS_open, 
                (long) syscall(SYS_virt2phys,file), 
			    (long) oflag, 
                (long) mode );

    return  syscall( SYS_open, fd );
}

weak_alias( __libc_open, open )
weak_alias( __libc_open, __open )
weak_alias( __libc_open, open64 )
weak_alias( __libc_open, __open64 )
weak_alias( __libc_open, __open_nocancel )


ssize_t __libc_write( int fd, const void *buf, size_t n )
{
    return _m5_syscall( SYS_write, 
            (long) syscall( SYS_fdmap, fd ), 
			(long) syscall( SYS_virt2phys, buf ), 
            (long) n );
}

weak_alias( __libc_write, write )
weak_alias( __libc_write, __write )
weak_alias( __libc_write, __write_nocancel )

ssize_t __libc_read( int fd, void *buf, size_t n )
{
    return _m5_syscall( SYS_read, 
            (long) syscall( SYS_fdmap, fd ),
			(long) syscall( SYS_virt2phys, buf ), 
            (long) n );
}

weak_alias( __libc_read, read )
weak_alias( __libc_read, __read )
weak_alias( __libc_read, __read_nocancel )

/* ----- stat.o ---------------- */

int __stat( const char *path, struct stat *buf )
{
    return __xstat( _STAT_VER, path, buf );
}

weak_alias( __stat, stat );

/* ----- stat64.o ---------------- */

int stat64( const char *path, struct stat64 *buf )
{
    return __xstat64( _STAT_VER, path, buf );
}

/* ----- fstat.o ---------------- */

int __fstat( int fd, struct stat *buf )
{
    return __fxstat( _STAT_VER, fd, buf );
}

weak_alias( __fstat, fstat );

/* ------ fstat64.o ------------------------ */

int __fstat64( int fd, struct stat *buf )
{
    return __fxstat64( _STAT_VER, fd , buf );
}

/* ------ xstat.o ------------------------ */

int __xstat(int ver, const char *path, struct stat *buf )
{
    int fd = open( path, O_RDONLY );
    if ( fd == -1 ) return -1;

    int ret = __fxstat( ver, fd, buf );

    close( fd );
    return ret;
}

int __xstat64(int ver, const char *path, struct stat64 *buf)
{
    int fd = open( path, O_RDONLY );
    if ( fd == -1 ) return -1;

    int ret = __fxstat64( ver, fd, buf );

    close( fd );
    return ret;
}

weak_alias( __xstat, _xstat );

/* ------ fxstat.o ------------------------ */

int __fxstat(int ver, int filedes, struct stat *buf )
{
    return _m5_syscall( SYS_fstat, 
                        (long) ver, 
                        (long) syscall( SYS_fdmap, filedes ), 
                        (long) syscall( SYS_virt2phys, buf) );
}

int __fxstat64(int ver, int filedes, struct stat64 *buf )
{
    return _m5_syscall( SYS_fstat64, 
                        (long) ver, 
                        (long) syscall( SYS_fdmap, filedes ), 
                        (long) syscall( SYS_virt2phys,buf) );
}

weak_alias( __fxstat, _fxstat );

int __ioctl( int fd, int request, ... )
{
    va_list ap;
    void* ptr = 0;

    if ( request == TCGETS ) {
        va_start( ap, request );
        ptr = va_arg(ap,void*) ;
        va_end( ap );
        ptr = (void*) syscall( SYS_virt2phys, ptr );
    }

    return _m5_syscall( SYS_ioctl, 
                        (long) syscall( SYS_fdmap, fd ), 
                        (long) request, 
                        (long) ptr );
}

weak_alias( __ioctl, ioctl )


int __tcgetattr(int fd, struct termios *termios_p)
{
    return ioctl( fd, TCGETS, termios_p ); 
}

weak_alias( __tcgetattr, tcgetattr )
