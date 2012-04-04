#include <stdio.h>
#include <stdlib.h>
#include <runtime.h>
#include <assert.h>

extern char **environ;
int main( int argc, char* argv[] )
{
    printf("hello mike\n");

    int rank = cnos_get_rank();
    int size = cnos_get_size();

    printf("rank=%d size=%d\n",rank,size);

    cnos_nidpid_map_t* map;

    assert( cnos_get_nidpid_map( &map ) == size );

    int i;
    for ( i = 0; i < size; i++) {
	printf("%d: rank=%d nid=%d pid=%d\n", rank, i, map[i].nid, map[i].pid );
    }

    printf("call cnos_barrier\n");
    cnos_barrier();
    printf("goodby mike\n");

    return 0;
}
