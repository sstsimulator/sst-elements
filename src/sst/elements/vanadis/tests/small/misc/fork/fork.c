
#include <stdio.h>
#include <unistd.h>
#include <syscall.h>

pid_t gettid() {
    return syscall(SYS_gettid);
}

int main( int argc, char* argv[] ) {

    printf("%s() pid=%d tid=%d ppid=%d pgid=%d\n",__func__, getpid(), gettid(), getppid(), getpgid(0));

    pid_t pid = fork();

    if ( 0 == pid ) {
    	printf("child: pid=%d tid=%d ppid=%d pgid=%d\n",getpid(), gettid(), getppid(), getpgid(0));
    } else {
        printf("parent: new child=%d\n",pid);
    }
} 
