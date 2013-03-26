#include <stdio.h>
#include <stdlib.h>

extern char **environ;
int main( int argc, char* argv[] )
{
    printf("hello mike pid=%d ppid=%d\n",getpid(),getppid());
    int i;
    for ( i = 0; i< argc; i++ ) {
        printf("argv[%d]=`%s`\n",i,argv[i]);
    }
    i = 0;
    while ( environ[i] ) {
        printf("envirn[%d]=`%s`\n",i,environ[i]);
        ++i;
    }
    printf("goodby mike\n");
    return 0;
}
