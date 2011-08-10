#include <m5rt.h>
#include <unistd.h>
#include <stdlib.h>
#include "m5_syscall.h"

int cnos_get_rank( void )
{
    char* tmp = getenv("RT_RANK");
    int rank = -1;
    if ( tmp ) {
         rank = atoi(tmp);
    }

    return rank;
}

int cnos_get_size( void )
{
    char* tmp = getenv("RT_SIZE");
    int rank = 0;
    if ( tmp ) {
         rank = atoi(tmp);
    }

    return rank;
}

int cnos_get_nidpid_map( cnos_nidpid_map_t** map )
{
    int size = cnos_get_size();
    if ( size > 0) {
        *map = (cnos_nidpid_map_t*) malloc( sizeof( cnos_nidpid_map_t ) * size );
        int i;
        for ( i = 0; i < size; i++ ) {
            (*map)[i].nid = i;  
            (*map)[i].pid = getpid();
        }
    }
    return size;
}

int cnos_barrier( void )
{
    return _m5_syscall(500,0,0,0);
}
