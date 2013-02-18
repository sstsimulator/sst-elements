#include <unistd.h>
#include <m5_syscall.h>
#include <syscall.h> // for SYS_exit

void exit( int status )
{
    _m5_syscall( SYS_exit, status, 0, 0 );
    while(1);
}

