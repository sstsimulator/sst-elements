#ifndef __m5_syscall_h
#define __m5_syscall_h

#ifdef __alpha__  
#define SYS_virt2phys 442
#define SYS_mmap_dev  443   
#elif __x86_64__
#define SYS_virt2phys 273 
#define SYS_mmap_dev  274 
#else
    #error "What's your cpu?"
#endif


extern long _m5_syscall( long number, long arg0, long arg1, long arg2 );

#endif
