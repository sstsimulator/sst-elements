#ifndef __m5_syscall_h
#define __m5_syscall_h

#ifdef __x86_64__
#define SYS_virt2phys 273 
#define SYS_mmap_dev  274 
#define SYS_barrier   500
#else
    #error "What's your cpu?"
#endif

extern long _m5_syscall( long number, long arg0, long arg1, long arg2 );

#endif
