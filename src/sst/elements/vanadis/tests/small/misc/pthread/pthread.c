#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>

#include <unistd.h>
#include <sys/syscall.h>

volatile int done = 0;

pid_t gettid() {
    return syscall(SYS_gettid);
}

#if 0
static inline unsigned __get_tp()
{
#if __mips_isa_rev < 2
        register unsigned tp __asm__("$3");
        __asm__ (".word 0x7c03e83b" : "=r" (tp) );
#else
        uint32_t tp;
        __asm__ ("rdhwr %0, $29" : "=r" (tp) );
#endif
        return tp;
    return 0;
}
#else
static inline unsigned __get_tp() {
    return 0;
}
#endif


static void* thread_start( void* arg ) {
    printf("%s() gettid()=%d getpid()=%d arg=%#x %lx\n",__func__,gettid(),getpid(),*(int*)arg,__get_tp());
    fflush(stdout);
    return 0;
}

static void* thread_start2( void* arg ) {
    printf("%s() gettid()=%d getpid()=%d arg=%#x %lx\n",__func__,gettid(),getpid(),*(int*)arg,__get_tp());
    fflush(stdout);
    return 0;
}

int main( int argc, char* argv[] ) 
{
    printf("%s() gettid()=%d getpid()=%d %lx\n",__func__,gettid(),getpid(), __get_tp());

    pthread_t self = pthread_self();

    pthread_attr_t attr;
    pthread_t thread_id;

    int arg = 0xdeadbeef;
    pthread_create( &thread_id, NULL, &thread_start, &arg);

    printf("after create thread_id=%p %lx\n",&thread_id,thread_id);

    void* ret;
    pthread_join(thread_id,&ret);
    printf("thread has exited\n");
    fflush(stdout);

    pthread_t thread_id2;

    arg = 0xf00df00d;
    pthread_create( &thread_id2, NULL, &thread_start2, &arg);

    pthread_join(thread_id2,&ret);
    printf("thread2 has exited\n");
} 
