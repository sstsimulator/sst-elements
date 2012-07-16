#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>


static void* threadMain( void *args )
{
    return NULL;
}

extern char **environ;
int main( int argc, char* argv[] )
{
    int numThreads = atoi( argv[1] );
    int i;
    pthread_t* threads = malloc( sizeof( *threads ) * numThreads );
    assert( threads );

    printf("numThreads %d\n",numThreads);

    for ( i = 0; i < numThreads; ++i ) {
        pthread_create( &threads[i], NULL, threadMain, NULL );
    } 

    for ( i = 0; i < numThreads; ++i ) {
        pthread_join( threads[i], NULL );
    }

    printf("test done\n");
    return 0;
}
