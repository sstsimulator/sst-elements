#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

int main(int argc, char* argv[] )
{
    int rank,size;
    printf("hello\n");
    MPI_Init( & argc, & argv );

    MPI_Comm_rank( MPI_COMM_WORLD, & rank );
    MPI_Comm_size( MPI_COMM_WORLD, & size );
    printf("after MPI_Init rank=%d size=%d\n",rank,size);

#if 1 
    MPI_Request request;
    MPI_Status status;
    char* str = "Hello World";
    char* buf = malloc( strlen(str) ); 
    MPI_Irecv( buf, strlen(str), MPI_CHAR, 0, 10, MPI_COMM_WORLD, &request ); 

    MPI_Send( str, strlen(str), MPI_CHAR, 0, 10, MPI_COMM_WORLD );
    MPI_Wait( &request, & status );
    printf("buf=`%s`\n",buf);
#endif
    MPI_Finalize();
    printf("after MPI_Finalize\n");
    return 0;
}
