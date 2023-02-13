#define _GNU_SOURCE
#include <sched.h>
#include <stdlib.h>

#include <stdio.h>
#include <unistd.h>

#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>

pid_t gettid() {
    return syscall(SYS_gettid);
}


int threadStart( void* arg ) {
    fprintf(stdout,"%s() gettid()=%d getpid()=%d arg=%#x\n",__func__,gettid(),getpid(),*(int*)arg);
    syscall(SYS_exit,0);
}

int main( int argc, char* argv[] ) 
{
    printf("%s() gettid()=%d getpid()=%d\n",__func__,gettid(),getpid());

    void* stack= malloc( 4096 ); 
    void* tls = malloc( 4096 * 1);

volatile int child_tid;

	int parent_tid = -1;
    int arg = 0xdeadbeef;
	child_tid = -1;

    unsigned flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND
                | CLONE_THREAD | CLONE_SYSVSEM | CLONE_SETTLS
                | CLONE_PARENT_SETTID | CLONE_CHILD_CLEARTID | CLONE_DETACHED;

    printf("threadEntry=%p stackPtr=%p argPtr=%p parentTidPtr=%p tls=%p childTidPtr=%p \n",
            threadStart, stack+4096, &arg, &parent_tid, tls, &child_tid);
    
    int ret = clone( threadStart, stack+4096, flags, &arg, &parent_tid, tls, &child_tid );

    printf("clone ret=%d, parentTidValue=%d\n",ret, parent_tid );

	while ( child_tid == -1 );

    printf("childTidValue=%d\n",child_tid);
} 
