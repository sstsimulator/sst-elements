#include <fcntl.h>

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>

int test( int dirfd, const char* path );

int main() {

//    test( -10, "/ascldap/users/mjleven/openat.test" ); 

    // this should fail
    test( -10, "/openat.test" );

    test( AT_FDCWD, "openat.test" );
    
	int dirfd = open("/tmp",O_RDONLY) ;
    printf("open('tmp') dirfd=%d\n",dirfd);
    test( dirfd, "openat.test" );
    if ( close(dirfd) ) {
        printf("close( %d ) failed, %s\n",dirfd,strerror(errno));
    }
    
    // this should be a bad dirFD
    test( 20, "openat.test" );
}

int test( int dirfd, const char* path )
{
	int mode = S_IRWXU;
	int fd = openat( dirfd, path, O_CREAT | O_RDWR | O_TRUNC, mode);

    if ( fd >= 0 ) {
        printf("openat(dirfd=%d, %s) returned %d\n",dirfd,path,fd);
        if ( close(fd) ) {
            printf("close( %d ) failed, %s\n",fd,strerror(errno));
        }
        if ( unlinkat(dirfd,path,0) ) {
            printf("unlink( %s ) failed, %s\n",path,strerror(errno));
        }
    } else {
	    printf("openat(dirfd=%d, %s) failed  `%s`\n",dirfd,path,strerror(errno));
    }
}
