#include <m5rt.h>
#include "m5_syscall.h"

int cnos_barrier( void )
{
    return _m5_syscall(500,0,0,0);
}
