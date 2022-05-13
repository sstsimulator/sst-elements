#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>

#define PATH_NAME "unlink-test-file"

int main( int argc, char* argv[] ) 
{
    if ( unlinkat( AT_FDCWD, PATH_NAME,0 ) ) {
        printf("unlinkat '%s' failed, error %s \n",PATH_NAME,strerror(errno));
    } else {
        printf("unlinkat '%s' succeeded\n",PATH_NAME);
    }   

    int fd = open( PATH_NAME, O_WRONLY|O_CREAT, S_IRWXU );
    if ( fd == -1) {
        printf("open '%s' failed, error %s \n",PATH_NAME,strerror(errno));
        exit(-1);
    }
    printf("open( '%s' ) succeeded\n",PATH_NAME);
    if ( unlinkat( AT_FDCWD, PATH_NAME, 0 ) ) {
        printf("unlinkat '%s' failed, error %s \n",PATH_NAME,strerror(errno));
    } else {
        printf("unlinkat '%s' succeeded\n",PATH_NAME);
    }
}

