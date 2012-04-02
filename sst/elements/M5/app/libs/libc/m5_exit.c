#include <sys/syscall.h>
#include <unistd.h>
#include "m5_syscall.h"

void exit( int status )
{
    _m5_syscall( SYS_exit, status, 0, 0 );
    while(1);
}

