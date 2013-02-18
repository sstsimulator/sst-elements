#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/syscall.h>

#define STR "hello Mike!"

int main( )
{
    int fd;
    //char buf[1024];
    char*  buf = malloc(1024);

    printf("hello mike\n");
    
    if ( ( fd = open("/tmp/hello", O_CREAT|O_RDWR, S_IRWXU ) ) == -1 ) {
	fprintf(stderr,"open 1 failed `%s`\n",strerror(errno));
	abort();
    }
 
    printf("fd=%d\n",fd);
    if ( write( fd, STR, strlen(STR) ) != strlen(STR) ) {
	fprintf(stderr,"write failed `%s`\n",strerror(errno));
	abort();
    }

    if ( close( fd ) == -1 ) {
	fprintf(stderr,"close 2 failed `%s`\n",strerror(errno));
	abort();
    }

    if ( ( fd = open("/tmp/hello", O_RDONLY ) ) == -1 ) {
	fprintf(stderr,"open 2 failed `%s`\n",strerror(errno));
	abort();
    }
 
    if ( read( fd, buf, strlen(STR) ) != strlen(STR) ) {
	fprintf(stderr,"read failed `%s`\n",strerror(errno));
	abort();
    }
 
    buf[strlen(STR)] = 0;

    printf("hello mike `%s`\n",buf);

    if ( close( fd ) == -1 ) {
	fprintf(stderr,"close 2 failed `%s`\n",strerror(errno));
	abort();
    }

    printf("goodbye mike\n");

    return 0;
}
