#include <m5rt.h>
#include <unistd.h>
#include <stdlib.h>

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
